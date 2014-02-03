/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "angort.h"
#include "file.h"
#include "ser.h"

void ClosureType::set(Value *v,Closure *c){
    v->clr();
    v->v.closure=c;
    v->t = Types::tClosure;
    incRef(v);
}

// this should ensure fixup save/resolution
void ClosureType::visitRefChildren(Value *v,ValueVisitor *visitor){
    Closure *c = v->v.closure;
    visitor->visit(NULL,&c->codeBlockValue);
    for(int i=0;i<c->ct;i++){
        c->table[i].receiveVisitor(visitor);
    }
}


void ClosureType::saveDataBlock(Serialiser *ser,const void *v){
    Closure *c = (Closure *)v;
    ser->file->write16(c->ct);
    for(int i=0;i<c->ct;i++){
        c->table[i].save(ser);
    }
    c->codeBlockValue.save(ser);
}

void *ClosureType::loadDataBlock(Serialiser *ser){
    int ct = ser->file->read16();
    Value *table = new Value[ct];
    
    for(int i=0;i<ct;i++){
        table[i].load(ser);
    }
    
    Closure *c = new Closure(NULL,ct,table);
    c->codeBlockValue.load(ser);
    
    return (void *)c;
}
