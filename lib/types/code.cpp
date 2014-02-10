/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "angort.h"

#include "ser.h"
#include "file.h"
#include "opcodes.h"

void CodeType::set(Value *v,const struct CodeBlock *cb){
    v->clr();
    v->v.cb=cb;
    v->t = Types::tCode;
}



void CodeType::saveValue(Serialiser *ser, Value *v){
    ser->file->write32(ser->getFixupByData(v->v.cb));
}
void CodeType::loadValue(Serialiser *ser, Value *v){
    v->setFixup(ser->file->read32());
}


void CodeType::saveDataBlock(Serialiser *ser,const void *v){
    const CodeBlock *cb = (CodeBlock *)v;
    cb->save(ser);
}

void CodeBlock::save(Serialiser *ser) const {
    File *f = ser->file;
    
    f->write16(size);
    f->write16(locals);
    f->write16(params);
    f->write16(closureMapCt);
    f->writeString(spec);
    
    // save closure map
    for(int i=0;i<closureMapCt;i++){
        f->write16(closureMap[i].parent);
        f->write16(closureMap[i].isLocalInParent ? 1 : 0);
    }
    
    // save instructions
    const Instruction *p = ip;
    for(int i=0;i<size;i++,p++){
        int opcode = p->opcode;
        // change the opcode for these, to indicate the data
        // is now a fixup index
        switch(p->opcode){
        case OP_LITERALSTRING:
            opcode = OP_LITERALSTRING_FIXUP;
            break;
        case OP_LITERALCODE:
            opcode = OP_LITERALCODE_FIXUP;
            break;
        default:break;
        }
        
        f->write16(opcode);
        
        switch(p->opcode){
        case OP_LITERALINT:
        case OP_CLOSUREGET:
        case OP_CLOSURESET:
        case OP_LOCALSET:
        case OP_LOCALGET:
        case OP_GLOBALSET:
        case OP_GLOBALGET:
        case OP_CONSTSET:
        case OP_CONSTGET:
        case OP_WORD:
        case OP_IF:
        case OP_LEAVE:
        case OP_IFLEAVE:
        case OP_DECLEAVENEG:
        case OP_ITERLEAVEIFDONE:
        case OP_JUMP:
            f->writeInt(p->d.i);
            break;
        case OP_PROPSET:
        case OP_PROPGET:
            f->writeString(ser->angort->props.getKey(p->d.prop));
            break;
        case OP_LITERALFLOAT:
            f->writeFloat(p->d.f);
            break;
        case OP_LITERALSTRING:
            f->write32(ser->getFixupByData(p->d.cb));
            break;
        case OP_LITERALCODE:
            f->write32(ser->getFixupByData(p->d.s));
            break;
        case OP_FUNC:
            f->writeString(ser->angort->funcs.getKey(p->d.func));
            break;
        default:
            break;
        }
    }
}


void *CodeType::loadDataBlock(Serialiser *ser){
    return (void *)new CodeBlock(ser);
 
}

CodeBlock::CodeBlock(Serialiser *ser){
    char buf[1024];
    
    File *f = ser->file;
    
    size = f->read16();
    locals = f->read16();
    params = f->read16();
    closureMapCt = f->read16();
    spec = f->readStringAlloc();
    
    printf("Reading %d, locals %d, params %d, closureMapCt %d\n",
           size,locals,params,closureMapCt);
    
    // load closure map
    closureMap = new ClosureMapEntry[closureMapCt];
    for(int i=0;i<closureMapCt;i++){
        closureMap[i].parent = f->read16();
        closureMap[i].isLocalInParent = (f->read16()==1);
    }
    
    // load instructions
    ip = new Instruction[size];
    Instruction *p = (Instruction *)ip;
    
    for(int i=0;i<size;i++,p++){
        p->opcode = f->read16();
        
        switch(p->opcode){
        case OP_LITERALINT:
        case OP_CLOSUREGET:
        case OP_CLOSURESET:
        case OP_LOCALSET:
        case OP_LOCALGET:
        case OP_GLOBALSET:
        case OP_GLOBALGET:
        case OP_CONSTSET:
        case OP_CONSTGET:
        case OP_WORD:
        case OP_IF:
        case OP_LEAVE:
        case OP_IFLEAVE:
        case OP_DECLEAVENEG:
        case OP_ITERLEAVEIFDONE:
        case OP_JUMP:
            p->d.i = f->readInt();
            break;
        case OP_PROPSET:
        case OP_PROPGET:
            f->readString(buf,1024);
            p->d.prop = ser->angort->props.get(buf);
            break;
        case OP_LITERALFLOAT:
            p->d.f = f->readFloat();
            break;
        case OP_LITERALCODE_FIXUP:
        case OP_LITERALSTRING_FIXUP:
            p->d.fixup = f->read32();
            break;
        case OP_FUNC:
            f->readString(buf,1024);
            p->d.func =  ser->angort->funcs.get(buf);
            break;
        case OP_LITERALSTRING:
        case OP_LITERALCODE:
            throw WTF; // should be a fixup!
        default:
            break;
        }
    }
            
    
}

