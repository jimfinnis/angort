/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "angort.h"
#include "cycle.h"


Closure::Closure(const CodeBlock *c,int tabsize,Value *t) : GarbageCollected() {
    if(c)
        Types::tCode->set(&codeBlockValue,c);
    CycleDetector::getInstance()->add(this);
    ct=tabsize;
    table=t;
//    printf("creating closure %p\n",this);
}

Closure::Closure(const Closure *c) : GarbageCollected() {
    throw RUNT("CLOSURE COPY DISALLOWED");
    codeBlockValue.copy(&c->codeBlockValue);
    ct = c->ct;
    table = new Value[ct];
    CycleDetector::getInstance()->add(this);
    for(int i=0;i<ct;i++)
        table[i].copy(c->table+i); // will INCREF the objects
//    printf("creating closure %p\n",this);
}

Closure::~Closure(){
    //    printf("closure deletion\n");
    delete [] table; // should delete AND DECREF the contained objects
    CycleDetector::getInstance()->remove(this);
//    printf("deleting closure %p\n",this);
}

void ClosureType::set(Value *v,Closure *c){
    v->clr();
    v->v.closure=c;
    v->t = Types::tClosure;
    incRef(v);
}

class ClosureIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    int idx; //!< current index
    Closure *c; //!< the range we're iterating over
    
public:
    /// create a list iterator for a list
    ClosureIterator(Closure *r){
        idx=0;
        c = r;
        c->incRefCt();
    }
    
    /// on destruction, delete the iterator
    virtual ~ClosureIterator(){
        if(c->decRefCt()){
            delete c;
        }
        v.clr();
    }
    
    /// set the current value to the first item
    virtual void first(){
        idx=0;
        if(idx<c->ct)
            v.copy(c->table+idx);
        else
            v.clr();
    }
    /// set the current value to the next item
    virtual void next(){
        idx++;
        if(idx<c->ct)
            v.copy(c->table+idx);
        else
            v.clr();
    }

    /// return true if we're out of bounds
    virtual bool isDone() const{
        return idx>=c->ct;
    }
    
    /// return the current value
    virtual Value *current(){
        return &v;
    }
};




Iterator<class Value *> *Closure::makeValueIterator(){
    return new ClosureIterator(this);
}


void ClosureType::visitRefChildren(Value *v,ValueVisitor *visitor){
    Closure *c = v->v.closure;
    visitor->visit(NULL,&c->codeBlockValue);
    for(int i=0;i<c->ct;i++){
        c->table[i].receiveVisitor(visitor);
    }
}


