#include "angort.h"
#include "opcodes.h"
#include "ser.h"
#include "file.h"

/// used for save/load identification
static uint32_t magicNumber = ANGORT_MAGIC;


/// type used for visiting the global tree, to create a table
/// of reference objects (the fixup table) before saving.

class BuildFixupTableVisitor : public ValueVisitor {
    Serialiser *ser;
    
public:
    BuildFixupTableVisitor(Serialiser *s){
        ser = s;
    }
    
    virtual bool visit(const char *globname,Value *v){
        if(v->t->isReference())
            ser->createFixup(v->t,v->v.v);
        if(v->t == Types::tCode)
            ser->createCodeFixups(v->v.cb);
    }
};

/// type used for visiting the global tree to resolve remaining
/// fixup data

class ResolveFixupTableVisitor : public ValueVisitor {
    Serialiser *ser;
    
public:
    ResolveFixupTableVisitor(Serialiser *s){
        ser = s;
    }
    
    virtual bool visit(const char *globname,Value *v){
        printf("resolve of %s, type %s\n",globname,v->t->name);
        if(v->t == Types::tFixup){
            FixupEnt *e = ser->getFixupByID(v->v.fixup);
            v->t = Type::findByID(e->t->id);
            v->v.v = (void *)e->v;
            v->incRef();
        }
        if(v->t == Types::tCode)
            ser->resolveCodeFixups((CodeBlock *)v->v.cb);
    }
    virtual void visit(const void *v){}
};

bool Serialiser::createFixup(Type *t,const void *v){
    if(isInFixups(v)){
        printf("Creating fixup : value %p already exists\n",v);
        return false;
    }
    
    printf("Creating fixup : %5d for type %10s, value %p\n",
           fixups->count(),
           t->name,
           v);
    FixupEnt *e = fixups->append();
    e->t=t;
    e->v=v;
    return true;
}

void Serialiser::createCodeFixups(const CodeBlock *c){
    const Instruction *ip = c->ip;
    
    printf("creating fixups in code block\n");
    
    for(int i=0;i<c->size;i++,ip++){
        angort->showop(ip,c->ip);printf("\n");
        switch(ip->opcode){
            /* these have references but use their own fixup system,
             * typically based on ID or name.
        case OP_PROPGET:
        case OP_PROPSET:
        case OP_GLOBALGET:
        case OP_GLOBALSET:
             */
        case OP_LITERALSTRING:
            createFixup(Types::tString,ip->d.s);
            break;
        case OP_LITERALCODE:
            if(createFixup(Types::tCode,ip->d.cb)){
                // and recurse!
                createCodeFixups(ip->d.cb);
            }
            break;
        default:
            break;
        }
    }
}
void Serialiser::resolveCodeFixups(CodeBlock *c){
    Instruction *ip = (Instruction *)c->ip; // remove const
    
    printf("resolving fixups in code block\n");
    
    for(int i=0;i<c->size;i++,ip++){
        angort->showop(ip,c->ip);printf("\n");
        switch(ip->opcode){
            /* these have references but use their own fixup system,
             * typically based on ID or name.
        case OP_PROPGET:
        case OP_PROPSET:
        case OP_GLOBALGET:
        case OP_GLOBALSET:
             */
        case OP_LITERALSTRING_FIXUP:
            // artifact of the way fixups work - the strings are loaded as blockallocked values, but
            // they're stored as plain strings in instructions.
            ip->d.s = ((const char*)resolveFixup(Types::tString,ip->d.fixup))+sizeof(BlockAllocHeader);
            printf("String fixup resolved: %s\n",ip->d.s);
            ip->opcode=OP_LITERALSTRING;
            break;
        case OP_LITERALCODE_FIXUP:
            ip->d.cb = (CodeBlock *)resolveFixup(Types::tCode,ip->d.fixup);
            ip->opcode=OP_LITERALCODE;
            resolveCodeFixups((CodeBlock *)ip->d.cb);
            break;
        default:
            break;
        }
    }
}

uint32_t Serialiser::getFixupByData(const void *v){
    // ugly
    for(int i=0;i<fixups->count();i++){
        FixupEnt *e = fixups->get(i);
        if(e->v == v)
            return i;
    }
    throw WTF;
}

void Serialiser::saveFixups(){
    file->writeInt(fixups->count());
    for(int i=0;i<fixups->count();i++){
        FixupEnt *e = fixups->get(i);
        file->write32(e->t->id); // type ID
        e->t->saveDataBlock(this,e->v);
    }
}

void Serialiser::loadFixups(){
    int ct = file->readInt();
    fixups = new ArrayList<FixupEnt>(ct);
    
    printf("loading %d fixups\n",ct);
    
    for(int i=0;i<ct;i++){
        FixupEnt *e = fixups->append();
        uint32_t t = file->read32();
        e->t = Type::findByID(t);
        printf("fixup %d: %s\n",i,e->t->name);
        if(!e->t)
            throw SerialisationException("").set("unknown type: %x",t);
        e->v = e->t->loadDataBlock(this);
    }
}

void Value::save(Serialiser *ser){
    ser->file->write32(t->id);
    t->saveValue(ser,this);
}


void Value::load(Serialiser *ser){
    Type *t = Type::findByID(ser->file->read32());
    if(!t)throw WTF;
    t->loadValue(ser,this);
}


void Serialiser::save(Angort *a, const char *name){
    angort = a;
    fixups = new ArrayList<FixupEnt>(256);
    
    // first, we iterate over the entire tree, building the fixup
    // map - a table linking referable objects (the object pointers,
    // not the values) with the objects to which they refer. Also
    // contains type data.
    
    ValueVisitor *v = new BuildFixupTableVisitor(this);
    a->visitGlobalData(v);
    delete(v);
    
    file = new File(name,true);
    
    // write the magic number and version
    file->write32(magicNumber);
    file->write32(ANGORT_VERSION);
    
    // now write the fixups
    saveFixups();
    // now the values
    a->globals.save(this);
    a->consts.save(this);
    a->words.save(this);
    
    
    delete file;
    delete fixups;
}

void Serialiser::load(Angort *a,const char *name){
    angort = a;
    file = new File(name,false);
    
    if(file->read32()!=magicNumber){
        delete file;file=NULL;
        throw SerialisationException("not an Angort image");
    }
    uint32_t d = file->read32();
    if(d!=ANGORT_VERSION){
        delete file;file=NULL;
        throw SerialisationException("").set("wrong version: file is version %d",d);
    }    
    a->clear(); // kind of necessary - deletes EVERYTHING
    
    // load the fixups
    loadFixups();
    // now the values
    a->globals.load(this);
    a->consts.load(this);
    a->words.load(this);
    
    
    // load done, resolve fixups
    ValueVisitor *v = new ResolveFixupTableVisitor(this);
    a->visitGlobalData(v);
    delete(v);
    
    delete file;file=NULL;
}


void Angort::loadImage(const char *name){
    Serialiser ser;
    ser.load(this,name);
}

