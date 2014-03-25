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
#include "cycle.h"

Closure::Closure(const CodeBlock *c,int tabsize,Value *t) : GarbageCollected() {
    if(c)
        Types::tCode->set(&codeBlockValue,c);
    CycleDetector::getInstance()->add(this);
    ct=tabsize;
    table=t;
    printf("creating closure %p\n",this);
}

Closure::Closure(const Closure *c) : GarbageCollected() {
    throw RUNT("CLOSURE COPY DISALLOWED");
    codeBlockValue.copy(&c->codeBlockValue);
    ct = c->ct;
    table = new Value[ct];
    CycleDetector::getInstance()->add(this);
    for(int i=0;i<ct;i++)
        table[i].copy(c->table+i); // will INCREF the objects
    printf("creating closure %p\n",this);
}

Closure::~Closure(){
    //    printf("closure deletion\n");
    delete [] table; // should delete AND DECREF the contained objects
    CycleDetector::getInstance()->remove(this);
    printf("deleting closure %p\n",this);
}

void ClosureType::set(Value *v,Closure *c){
    v->clr();
    v->v.closure=c;
    v->t = Types::tClosure;
    printf("Closure not yet traced!\n");
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
