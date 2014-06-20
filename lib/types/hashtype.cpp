/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "angort.h"
#include "hash.h"
#include "cycle.h"

HashObject::HashObject(){
    hash = new Hash();
    CycleDetector::getInstance()->add(this);
}

HashObject::~HashObject(){
    CycleDetector::getInstance()->remove(this);
    delete hash;
}

Iterator<Value *> *HashObject::makeValueIterator() {
    return hash->createIterator(false);
}

Iterator<Value *> *HashObject::makeKeyIterator() {
    return hash->createIterator(true);
}


Hash *HashType::set(Value *v){
    v->clr();
    v->t = this;
    HashObject *h = new HashObject();
    v->v.hash = h;
    incRef(v);
    return h->hash;
}

Hash *HashType::get(Value *v){
    if(v->t != this)
        throw RUNT("not a hash");
    return v->v.hash->hash;
}

void HashType::setValue(Value *coll,Value *k,Value *v){
    Hash *h = coll->v.hash->hash;
    h->set(k,v);
}

void HashType::getValue(Value *coll,Value *k,Value *result){
    Hash *h = coll->v.hash->hash;
    if(h->find(k))
        result->copy(h->getval());
    else
        result->clr();
}

int HashType::getCount(Value *coll){
    Hash *h = coll->v.hash->hash;
    return h->count();
}

void HashType::removeAndReturn(Value *coll,Value *k,Value *result){
    Hash *h = coll->v.hash->hash;
    if(h->find(k)){
        result->copy(h->getval());
        h->del(k);
    } else
        result->clr();
}

bool HashType::isIn(Value *coll,Value *item){
    Hash *h = coll->v.hash->hash;
    if(h->find(item))
        return true;
    else
        return false;
}

void HashType::clone(Value *out,const Value *in){
    HashObject *p = new HashObject();
    Hash *h = get(const_cast<Value *>(in));
    
    // cast away constness - makeIterator() can't be const
    // because it modifies refcounts
    Iterator<Value *> *iter = makeIterator(
                                           const_cast<Value *>(in));
    for(iter->first();!iter->isDone();iter->next()){
        Value *k = iter->current();
        Value *v;
        if(h->find(k))
            v = h->getval();
        else
            throw WTF;
        p->hash->set(k,v);
 
    }
    
    out->clr();
    out->t = this;
    out->v.hash = p;
    incRef(out);
}
