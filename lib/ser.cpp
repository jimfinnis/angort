#include "angort.h"
#include "opcodes.h"
#include "ser.h"
#include "file.h"
#include "hash.h"

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
        return true; // recurse
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
//        printf("resolve of %s, type %s to ",globname?globname:"[unnamed]",v->t->name);
        if(v->t == Types::tFixup){
            FixupEnt *e = ser->getFixupByID(v->v.fixup);
            v->t = Type::findByID(e->t->id);
            v->v.v = (void *)e->v;
            v->incRef();
            
//            char buf[1024];
//            printf("%s: %s\n",v->t->name,v->toString(buf,1024));

        }
//        else
//              printf("\n");
        if(v->t == Types::tCode)
            ser->resolveCodeFixups((CodeBlock *)v->v.cb);
        return true; // recurse
    }
    virtual void visit(const void *v){}
};

bool Serialiser::createFixup(Type *t,const void *v){
    if(isInFixups(v)){
//        printf("Creating fixup : value %p already exists\n",v);
        return false;
    }
    
/*    printf("Creating fixup: %5d for type %10s, value %p\n",
         fixups->count(),
         t->name,
         v);
 */
    FixupEnt *e = fixups->append();
    e->t=t;
    e->v=v;
//    printf("Fixup: val %p -> %p\n",e,e->v);
    return true;
}

void Serialiser::createCodeFixups(const CodeBlock *c){
    const Instruction *ip = c->ip;
    
//    printf("creating fixups in code block\n");
    
    for(int i=0;i<c->size;i++,ip++){
//        angort->showop(ip,c->ip);printf("\n");
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
    
//    printf("resolving fixups in code block\n");
    
    for(int i=0;i<c->size;i++,ip++){
//        angort->showop(ip,c->ip);printf("\n");
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
//            printf("String fixup resolved: %s\n",ip->d.s);
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
//    printf("looking for fixup for %p\n",v);
    for(int i=0;i<fixups->count();i++){
        FixupEnt *e = fixups->get(i);
//        printf("  checking %d: %p\n",i,e->v);
        if(e->v == v)
            return i;
    }
//    printf("NOT FOUND\n");
    throw WTF;
}

void Serialiser::saveFixups(){
    file->writeInt(fixups->count());
    
    FixupEnt *snark;
    if(fixups->count()>10)
        snark = fixups->get(10);
    
    for(int i=0;i<fixups->count();i++){
        FixupEnt *e = fixups->get(i);
//        printf("Saving fixup %d, %p -> %p, type=%p\n",i,e,e->v,e->t);
        file->write32(e->t->id); // type ID
        e->t->saveDataBlock(this,e->v);
    }
}

void Serialiser::loadFixups(){
    int ct = file->readInt();
    fixups = new ArrayList<FixupEnt>(ct);
    
//    printf("loading %d fixups\n",ct);
    
    for(int i=0;i<ct;i++){
        FixupEnt *e = fixups->append();
        uint32_t t = file->read32();
        e->t = Type::findByID(t);
//        printf("fixup %d: %s\n",i,e->t->name);
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
    delete v;
    
    file = new File(name,true);
    
    // write the magic number and version
    file->write32(magicNumber);
    file->write32(a->getVersion());
    
    // now write the fixups
    saveFixups();
    // now the values
    a->names.save(this);
    
    delete file;
    delete fixups;fixups=NULL;
}

void Serialiser::load(Angort *a,const char *name){
    angort = a;
    file = new File(name,false);
    
    if(file->read32()!=magicNumber){
        delete file;file=NULL;
        throw SerialisationException("not an Angort image");
    }
    uint32_t d = file->read32();
    if(d!=a->getVersion()){
        delete file;file=NULL;
        throw SerialisationException("").set("wrong version: file is version %d",d);
    }    
    a->clear(); // kind of necessary - deletes EVERYTHING
    
    // load the fixups
    loadFixups();
    // now the values
    a->names.load(this);
    
    // load done, resolve fixups
    ValueVisitor *v = new ResolveFixupTableVisitor(this);
    a->visitGlobalData(v);
    delete(v);
    delete fixups;fixups=NULL;
    delete file;file=NULL;
}


void Angort::loadImage(const char *name){
    Serialiser ser;
    ser.load(this,name);
}

void Hash::save(Serialiser *ser){
    HashEnt *ent=table;
    
    ser->file->write32(mask);
    ser->file->write32(used);
    ser->file->write32(fill);
    
    
    for(unsigned int i=0;i<mask+1;i++,ent++){
        if(ent->isUsed()){
            char buf[1024];
//            printf("Index: %5d ",i);
//            printf("Hash: %10d ",ent->hash);
//            printf("Key: %20s ",ent->k.toString(buf,1024));
//            printf("Val: %20s\n",ent->v.toString(buf,1024));
            
            ser->file->write32(i);
            ser->file->write32(ent->hash);
            ent->k.save(ser);
            ent->v.save(ser);
        }
    }
}

void Hash::load(Serialiser *ser){
    mask = ser->file->read32();
    used = ser->file->read32();
    fill = ser->file->read32();
    
    delete [] table;
    table = new HashEnt[mask+1];
    
    for(unsigned int i=0;i<used;i++){
        uint32_t idx = ser->file->read32();
        HashEnt *ent = table+idx;
        ent->hash = ser->file->read32();
        
        ent->k.load(ser);
        ent->v.load(ser);
        
//        char buf[1024];
//        printf("Index: %5d ",idx);
//        printf("Hash: %10d ",ent->hash);
//        printf("Key: %20s ",ent->k.toString(buf,1024));
//        printf("Val: %20s\n",ent->v.toString(buf,1024));
    }
}
