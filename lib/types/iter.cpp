/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"

namespace angort {

IteratorObject::IteratorObject(Iterator<Value *> *iter, Value *src) : GarbageCollected ("iter") {
    iterator = iter;
    iterable = new Value;
    current = new Value;
    iterable->copy(src);
}

IteratorObject::~IteratorObject(){
    delete iterable;
    delete iterator;
    delete current;
}


void IteratorType::set(Value *v,Value *src,Iterator<Value *> *iter){
    v->clr();
    v->v.gc = new IteratorObject(iter,src);
    v->t = Types::tIter;
    incRef(v);
}

Iterator<Value *> *IteratorType::get(Value *v){
    if(v->t!=this)
        throw RUNT("ex$noiter","not an iterator");
    IteratorObject *i = (IteratorObject *)v->v.gc;
    return i->iterator;
}


}
