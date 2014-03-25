/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "angort.h"
#include "file.h"
#include "ser.h"
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


Iterator<Value *> *HashType::makeIterator(Value *v){
    return v->v.hash->hash->createIterator();
}

void HashType::saveDataBlock(Serialiser *ser,const void *v){
    HashObject *r = (HashObject *)v;
    r->hash->save(ser);
}
void *HashType::loadDataBlock(Serialiser *ser){
    HashObject *r = new HashObject();
    r->hash->load(ser);
    return (void *)r;
}


void HashType::visitRefChildren(Value *v,ValueVisitor *visitor){
    v->v.hash->hash->visitRefChildren(visitor);
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
