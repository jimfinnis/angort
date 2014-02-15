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

HashObject::HashObject(){
    hash = new Hash();
}

HashObject::~HashObject(){
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


Iterator<Value *> *HashType::makeValueIterator(Value *v){
    return v->v.hash->hash->createValueIterator();
}
Iterator<Value *> *HashType::makeKeyIterator(Value *v){
    return v->v.hash->hash->createKeyIterator();
}

void HashType::saveDataBlock(Serialiser *ser,const void *v){
    throw WTF;
    HashObject *r = (HashObject *)v;
}
void *HashType::loadDataBlock(Serialiser *ser){
    HashObject *r = new HashObject();
    throw WTF;
    return (void *)r;
}


void HashType::visitRefChildren(Value *v,ValueVisitor *visitor){
    Hash *h = v->v.hash->hash;
    
    Iterator<Value *> *iter=h->createKeyIterator();
    
    for(iter->first();!iter->isDone();iter->next()){
        // first visit the key
        Value *v = iter->current();
        v->receiveVisitor(visitor);
        // then the value
        if(h->find(v))
            h->getval()->receiveVisitor(visitor);
        else
            throw WTF;
    }
    delete iter;
}
