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


//Closure::Closure(const CodeBlock *c,int tabsize,Value *t) : GarbageCollected() {
//    printf("creating closure %p\n",this);
//}

//Closure::~Closure(){
//    printf("deleting closure %p\n",this);
//}

void ClosureType::set(Value *v,Closure *c){
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
//        c->incRefCt();
    }
    
    /// on destruction, delete the iterator
    virtual ~ClosureIterator(){
//        if(c->decRefCt()){
//            delete c;
//        }
        v.clr();
    }
    
    /// set the current value to the first item
    virtual void first(){
        idx=0;
/*        if(idx<c->ct)
            v.copy(c->table+idx);
        else
*/            v.clr();
    }
    /// set the current value to the next item
    virtual void next(){
        idx++;
/*        if(idx<c->ct)
            v.copy(c->table+idx);
        else
*/            v.clr();
    }

    /// return true if we're out of bounds
    virtual bool isDone() const{
        //        return idx>=c->ct;
        return true;
    }
    
    /// return the current value
    virtual Value *current(){
        return &v;
    }
};




//Iterator<class Value *> *Closure::makeValueIterator(){
//    return new ClosureIterator(this);
//}

}
