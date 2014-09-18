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


Closure::Closure(Closure *p) : GarbageCollected() {
    parent = p;
    if(p)p->incRefCt();
    printf("allocating closure %p, parent %p\n",this,parent);
}

void Closure::init(const CodeBlock *c){
    printf("creating closure %p, parent %p\n",this,parent);
    
    printf("Chain:\n");
    for(Closure *qq=parent;qq;qq=qq->parent)
        printf("  - %p\n",qq);
    
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
    
    for(int i=0;i<cb->closureTableSize;i++){
        // iterate through each item, finding
        // the closure and thus the block in
        // the return stack
        int lev = cb->closureTable[i].levelsUp;
        int idx = cb->closureTable[i].idx;
        
        // this is another closure (or possibly
        // the same one) whose block contains the
        // value we want. We walk the appropriate
        // number of levels up the parent chain
        // to find it.
        
        printf("Building map for %p. Looking for lev %d, idx %d\n",this,
               lev,idx);
        
        Closure *reffed=this;
        for(int j=0;j<lev;j++){
            reffed=reffed->parent;
            printf("  Ref jump %p\n",reffed);
        }
        
        if(!reffed->block)throw WTF;
        map[i] = reffed->block+idx;
        
        printf("  Value currently %s\n",map[i]->toString().get());
        
        // and we increment the refcount on the block
        // if the block is in a different closure
        if(this!=reffed){
            blocksUsed[i] = reffed;
            reffed->incRefCt();
        } else blocksUsed[i]=NULL; // self-refs are NULL
    }    
}


Closure::~Closure(){
    printf("deleting closure %p\n",this);
    if(block)delete [] block;
    if(parent && parent->decRefCt())
        delete parent;
    
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
    }
    
    /// set the current value to the first item
    virtual void first(){
        idx=0;
        if(idx<c->cb->closureBlockSize){
            if(c->blocksUsed[idx]){
                v.t = Types::tClosure;
                v.v.closure = c->blocksUsed[idx];
            } else
                v.t = Types::tNone;
        } else
            v.t = Types::tNone;
    }
    /// set the current value to the next item
    virtual void next(){
        idx++;
        if(idx<c->cb->closureBlockSize){
            if(c->blocksUsed[idx]){
                v.t = Types::tClosure;
                v.v.closure = c->blocksUsed[idx];
            } else
                v.t = Types::tNone;
        } else
            v.t = Types::tNone;
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
    
    if(parent)
        parent->show("Parent of previous");
          
}

Iterator<class Value *> *Closure::makeValueIterator(){
    return new ClosureIterator(this);
}

}
