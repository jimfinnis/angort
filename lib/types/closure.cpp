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
    
    // this should never happen because new closures
    // should not be created for bare codeblocks.
    if(!cb->closureTableSize)throw WTF;
    
    // easy part done, now to create the map
    // from the table
    
    map = new Value * [cb->closureTableSize];
    blocksUsed = new Closure * [cb->closureTableSize];
    
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
        
        Value *reffed = a->getClosureForLevel(lev);
        map[i] = reffed->v.closure->block+idx;
        
        // and we increment the refcount on the block
        // if the block is in a different closure
        if(this!=reffed->v.closure){
            blocksUsed[i] = reffed->v.closure;
            reffed->incRef();
        } else blocksUsed[i]=NULL; // self-refs are NULL
    }    
}


Closure::~Closure(){
    printf("deleting closure %p\n",this);
    if(block)delete [] block;
    
    // dereference the blocks we have access to
    for(int i=0;i<cb->closureTableSize;i++){
        printf("decrementing referenced closure\n  ");
        if(blocksUsed[i]){ // self-ref doesn't count (see above)
            if(blocksUsed[i]->decRefCt())
                delete blocksUsed[i];
        }
        printf("done decrementing referenced closure\n");
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
