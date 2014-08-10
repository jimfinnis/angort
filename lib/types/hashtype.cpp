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
#include "types/symbol.h"

namespace angort {

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

void HashType::clone(Value *out,const Value *in,bool deep){
    HashObject *p = new HashObject();
    Hash *h = get(const_cast<Value *>(in));
    
    // cast away constness - makeIterator() can't be const
    // because it modifies refcounts
    
    HashKeyIterator iter(h);
    for(iter.first();!iter.isDone();iter.next()){
        Value *k = iter.current();
        Value *v;
        if(h->find(k))
            v = h->getval();
        else
            throw WTF;
        
        // because collections can't be keys, we only need worry
        // about the value for deep cloning.
        
        if(deep){
            Value deepv;
            v->t->clone(&deepv,v);
            p->hash->set(k,&deepv); // hash will copy the value
        } else {
            p->hash->set(k,v);
        }
        
 
    }
    
    out->clr();
    out->t = this;
    out->v.hash = p;
    incRef(out);
}


const char *HashType::toString(bool *allocated,const Value *v) const {
    // first, does the hash contain the toString symbol key?
    
    static int symbKey=-1;
    if(symbKey<0)
        symbKey = Types::tSymbol->getSymbol("toString");
    
    Value k;
    Types::tSymbol->set(&k,symbKey);
    
    Hash *h = v->v.hash->hash;
    if(h->find(&k)){
        // is it a function?
        Value *outval = h->getval();
        if(outval->t->isCallable()){
            Angort *a = Angort::getCallingInstance();
            // Yes, so call the function and get the returned value
            a->pushval()->copy(v);
            a->runValue(outval);
            outval = a->popval();
            return outval->t->toString(allocated,outval);
        } else {
            // if not, just turn it into a string and use that
            return outval->t->toString(allocated,outval);
        }
    }
        
    // otherwise, do the default operation
    return Type::toString(allocated,v);
}

}
