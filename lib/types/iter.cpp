/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"

IteratorObject::IteratorObject(Iterator<Value *> *iter, Value *src) {
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
    IteratorObject *i = (IteratorObject *)v->v.gc;
    return i->iterator;
}
