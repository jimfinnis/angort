/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "angort.h"
#include "cycle.h"

namespace angort {


Closure::Closure() : GarbageCollected() {
}

void Closure::init(const CodeBlock *c){
    printf("creating closure %p\n",this);
    cb = c;
    
    if(cb->closureBlockSize)
        block = new Value[cb->closureBlockSize];
    else
        block = NULL;
    
    // easy part done, now to create the map.
    
    // this should never happen because new closures
    // should not be created for bare codeblocks.
    if(!cb->closureTableSize)throw WTF;
    map = new Value * [cb->closureTableSize];
    blocksUsed = new Value * [cb->closureTableSize];
    
    // we're going to need to get stuff from Angort's
    // global return stack to find the blocks
    
    Angort *a = Angort::getCallingInstance();
    
    for(int i=0;i<cb->closureTableSize;i++){
        // iterate through each item, finding
        // the closure and thus the block in
        // the return stack
        int lev = cb->closureTable[i].levelsUp;
        int idx = cb->closureTable[i].idx;
        
        // this is another closure (or possibly
        // the same one) whose block contains the
        // value we want
        
        Value *block = a->getClosureForLevel(lev);
        map[i] = block->v.closure->block+idx;
        blocksUsed[i] = block;
        
        // and we increment the refcount on the block
        block->incRef();
    }    
}


Closure::~Closure(){
    printf("deleting closure %p\n",this);
    if(block)delete [] block;
    
    // dereference the blocks we have access to
    for(int i=0;i<cb->closureTableSize;i++){
        blocksUsed[i]->decRef();
    }
        
    delete[] map;
}

void ClosureType::set(Value *v, Closure *c){
    v->clr();
    v->v.closure=c;
    v->t = Types::tClosure;
    incRef(v);
}

class ClosureIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    int idx; //!< current index
    struct Closure *c; //!< the range we're iterating over
    
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
        if(idx<c->cb->closureBlockSize)
            v.copy(c->block+idx);
        else
            v.clr();
    }
    /// set the current value to the next item
    virtual void next(){
        idx++;
        if(idx<c->cb->closureBlockSize)
            v.copy(c->block+idx);
        else
            v.clr();
    }

    /// return true if we're out of bounds
    virtual bool isDone() const{
        return idx>=c->cb->closureBlockSize;
    }
    
    /// return the current value
    virtual Value *current(){
        return &v;
    }
};


void Closure::show(const char *s){
    printf("Closure %s at %p: block %s\n",s,this,block?"Y":"N");
    printf("Block:\n");
    for(int i=0;i<cb->closureBlockSize;i++){
        printf("  %2d : %p %s\n",i,block+i,block[i].toString().get());
    }
    printf("Map:\n");
    for(int i=0;i<cb->closureTableSize;i++){
        printf("  %2d : %p %s\n",i,map[i],map[i]->toString().get());
    }
}

Iterator<class Value *> *Closure::makeValueIterator(){
    return new ClosureIterator(this);
}

}
