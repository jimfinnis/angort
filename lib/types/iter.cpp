/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"

IteratorObject::IteratorObject(Iterator<Value *> *iter) {
    iterator = iter;
    current = new Value;
}


void IteratorType::set(Value *v,Iterator<Value *> *iter){
    v->clr();
    v->t = this;
    v->v.gc = new IteratorObject(iter);
    incRef(v);
    
}

Iterator<Value *> *IteratorType::get(Value *v){
    IteratorObject *i = (IteratorObject *)v->v.gc;
    return i->iterator;
}
    
    
