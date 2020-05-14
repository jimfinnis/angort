/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "angort.h"
#include "cycle.h"

//#define CLOSUREDEBUG

static void closprintf(const char *s,...){
#ifdef CLOSUREDEBUG
    char buf[256];
    va_list args;
    va_start(args,s);
    vsprintf(buf,s,args);
    printf("[context %p with cb %p]  %s\n",this,cb,buf);
#endif
}

namespace angort {

Closure::Closure(Closure *p) : GarbageCollected("closure") {
    parent = p;
    ip = NULL;
    if(p)p->incRefCt();
    closprintf("allocating closure %p, parent %p\n",this,parent);
}

void Closure::init(const CodeBlock *c){
    cb = c;
    
    closprintf("creating closure %p for codeblock %p, parent closure %p\n",this,cb,parent);
#ifdef CLOSUREDEBUG    
    printf("Chain:\n");
    for(Closure *qq=parent;qq;qq=qq->parent)
        printf("  - %p\n",qq);
#endif    
    if(cb->closureBlockSize)
        block = new Value[cb->closureBlockSize];
    else
        block = NULL;
    
    // this should never happen because new closures
    // should not be created for bare codeblocks (even dummy closures
    // have a single closure table entry)
    
    if(!cb->closureTableSize)throw WTF;
    
    // easy part done, now to create the map
    // from the table
    
    map = new Value * [cb->closureTableSize];
    blocksUsed = new Closure * [cb->closureTableSize];
    
    closprintf("Closure %p has %d entries\n",this,cb->closureTableSize);
    
    for(int i=0;i<cb->closureTableSize;i++){
        // iterate through each item, finding
        // the closure and thus the block in
        // the return stack
        int lev = cb->closureTable[i].levelsUp;
        int idx = cb->closureTable[i].idx;
        
        if(lev<0){
            // in this case, this is part of a dummy closure. We just
            // do the minimum we can get away with, which is create
            // a dummy map entry.
            blocksUsed[i]=NULL;
            map[i] = NULL;
        } else {
            
            // this is another closure (or possibly
            // the same one) whose block contains the
            // value we want. We walk the appropriate
            // number of levels up the parent chain
            // to find it.
            
            closprintf("Building map for %p. Looking for lev %d, idx %d\n",this,lev,idx);
            
            Closure *reffed=this;
            for(int j=0;j<lev;j++){
                closprintf("  Reffed closure block level %d is %p, parent %p\n",j,reffed,reffed->parent);
                if(!reffed){
                    // An error has occurred.
                    if(block){
                        block = NULL;
                        delete[]  block;
                    }
                    delete[] map;
                    delete[] blocksUsed;
                    blocksUsed =NULL;
                    map=NULL;
                    // note (a) - can leave a nasty closure which needs destroying carefully at note (b)
                    throw RUNT("ex$closure","Attempt to close over variable in unavailable containing block");
                }
                reffed=reffed->parent;
            }
            if(!reffed)
                throw RUNT(EX_WTF,"closure init WTF: reffed closure is null");
            if(!reffed->block)
                throw RUNT(EX_WTF,"closure init WTF: reffed closure's codeblock is null");
            map[i] = reffed->block+idx;
            
            closprintf("  Value currently %s\n",map[i]->toString().get());
            
            // and we increment the refcount on the block
            // if the block is in a different closure
            if(this!=reffed){
                blocksUsed[i] = reffed;
                reffed->incRefCt();
            } else blocksUsed[i]=NULL; // self-refs are NULL
        }    
    }
}

void Closure::wipeContents(){
    for(int i=0;i<cb->closureBlockSize;i++)
        block[i].wipeIfInGCCycle();
}


Closure::~Closure(){
    closprintf("deleting closure %p\n",this);
    if(block)delete [] block;
    if(!CycleDetector::getInstance()->isInDeleteCycle()){
        // we do not do recursive deletion if we're deleting
        // stuff in a left-over cycle. Deletions which need
        // to be done will be done by the code in the cycle
        // detector.
        
        if(parent && parent->decRefCt())
            delete parent;
        
        // dereference the blocks we have access to
        if(blocksUsed){ // (b) could be null for a bad closure (see note (a) above)
            for(int i=0;i<cb->closureTableSize;i++){
                closprintf("decrementing referenced closure\n  ");
                // self-ref doesn't count (see above)
                if(blocksUsed[i] && blocksUsed[i]->decRefCt())
                    delete blocksUsed[i];
                closprintf("done decrementing referenced closure\n");
            }
        }
    }
    
    delete [] blocksUsed;
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
    ClosureIterator(const Closure *r){
        idx=0;
        c = (Closure *)r;
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
    
    virtual int index() const {
        return idx;
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


void Closure::clearZombieReferences(){
    for(int i=0;i<cb->closureTableSize;i++){
        Value *v = map[i];
        if(GarbageCollected *g = v->t->getGC(v)){
            if(g->gc_refs == 0xffff) // if child not done
                v->init(); // clear without any reference count changes
        }
    }
}

void Closure::decReferentsCycleRefCounts(){
    // we have to do the references of the things stored in the closure (covered by the iterator),
    // the closure blocks referred to, and the parent
    
    for(int i=0;i<cb->closureTableSize;i++){
        closprintf("Decrementing %p\n",blocksUsed[i]);
        if(blocksUsed[i]){
            blocksUsed[i]->gc_refs--;
            closprintf("decrementing cycle count for block use on %p, now %d\n",blocksUsed[i],
                       blocksUsed[i]->gc_refs);
        }
    }
    if(parent)
        parent->gc_refs--;
}

void Closure::traceAndMove(class CycleDetector *cycle){
    GarbageCollected *g;
    for(int i=0;i<cb->closureTableSize;i++){
        if(GarbageCollected *g = blocksUsed[i]){
            if(!g->gc_refs) { // if child not done
                cycle->move(g);
                cycle->traceAndMoveIterator(g,true);
                cycle->traceAndMoveIterator(g,false);
                g->traceAndMove(cycle);
            }
        }
    }
    if(parent && !parent->gc_refs){
        g=parent;
        if(!g->gc_refs) { // if child not done
            cycle->move(g);
            cycle->traceAndMoveIterator(g,true);
            cycle->traceAndMoveIterator(g,false);
            g->traceAndMove(cycle);
        }
    }
    
}

void Closure::show(const char *s){
    printf("%s of Closure %p: is a block: %s\n",s,this,block?"Y":"N");
    printf("Block:\n");
    for(int i=0;i<cb->closureBlockSize;i++){
        printf("  %2d : %p %s\n",i,block+i,block[i].toString().get());
    }
    printf("Map:\n");
    for(int i=0;i<cb->closureTableSize;i++){
        printf("  %2d : %p val=%s\n",i,map[i],map[i]->toString().get());
        if(blocksUsed[i])
            printf("     : contained in closure %p\n",blocksUsed[i]);
        else
            printf("     : contained in self\n");
    }
    
    if(parent)
        parent->show("Parent of previous");
}


Iterator<class Value *> *Closure::makeValueIterator()const{
    return new ClosureIterator(this);
}

}
