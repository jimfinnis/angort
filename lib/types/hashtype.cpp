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

HashObject::HashObject() : GarbageCollected("hash") {
    hash = new Hash();
}

HashObject::~HashObject(){
    delete hash;
}

void HashObject::wipeContents(){
    // key's can't be GC.
    HashValueIterator iter(hash);
    for(iter.first();!iter.isDone();iter.next()){
        iter.current()->wipeIfInGCCycle();
    }
}

// this is unpleasant, but happened because the underlying iterators don't
// know about the value to lock it. It also encapsulates the hash iterator
// thread lock, which works the same way as in lists.

class HashObjectIterator : public Iterator<Value *>{
    HashObject *h;
    Iterator<Value *> *iter; // the underlying iterator
    ReadLock *lock;
public:
    HashObjectIterator(const HashObject *ho,bool iskeyiterator){
        h = (HashObject *)ho;
        h->incRefCt();
        lock=NULL;
        if(iskeyiterator)iter = new HashKeyIterator(h->hash);
        else iter = new HashValueIterator(h->hash);
    }
    
    ~HashObjectIterator(){
        delete iter;
        if(h->decRefCt())
            delete h;
        if(lock){delete lock; lock=NULL;}
    }
    
    virtual void first() {
        if(lock)delete lock;
        lock = new ReadLock(h->hash);
        iter->first();
        if(iter->isDone()){
            delete lock;
            lock=NULL;
        }
    }
    virtual void next() {
        iter->next();
        if(iter->isDone()){
            if(lock){delete lock; lock=NULL;}
        }
    }
    virtual bool isDone()const {
        return iter->isDone();
    }
    virtual int index() const {
        return iter->index();
    }
    virtual Value *current() {
        return iter->current();
    }
    
};


Lockable *HashType::getLockable(Value *v) const{
    return get(v);
}

Iterator<Value *> *HashObject::makeValueIterator() const{
    ReadLock lock(hash);
    return new HashObjectIterator(this,false);
}

Iterator<Value *> *HashObject::makeKeyIterator() const {
    ReadLock lock(hash);
    return new HashObjectIterator(this,true);
}


Hash *HashType::set(Value *v)const{
    v->clr();
    v->t = this;
    HashObject *h = new HashObject();
    v->v.hash = h;
    incRef(v);
    return h->hash;
}

void HashType::set(Value *v,HashObject *ho)const{
    v->clr();
    v->t = this;
    v->v.hash = ho;
    incRef(v);
}


Hash *HashType::get(Value *v)const{
    if(v->t != this)
        throw RUNT("ex$nohash","").set("not a hash, is a %s",v->t->name);
    return v->v.hash->hash;
}

void HashType::setValue(Value *coll,Value *k,Value *v)const{
    if(coll->t != this)
        throw RUNT("ex$nohash","").set("not a hash, is a %s",coll->t->name);
    Hash *h = coll->v.hash->hash;
    WriteLock lock = WL(h);
    h->set(k,v);
}

void HashType::getValue(Value *coll,Value *k,Value *result)const{
    Hash *h = coll->v.hash->hash;
    ReadLock lock(h);
    if(h->find(k))
        result->copy(h->getval());
    else
        result->clr();
}

int HashType::getCount(Value *coll)const{
    Hash *h = coll->v.hash->hash;
    ReadLock lock(h);
    return h->count();
}

void HashType::removeAndReturn(Value *coll,Value *k,Value *result)const{
    Hash *h = coll->v.hash->hash;
    WriteLock lock = WL(h);
    if(h->find(k)){
        result->copy(h->getval());
        h->del(k);
    } else
        result->clr();
}

bool HashType::isIn(Value *coll,Value *item)const{
    Hash *h = coll->v.hash->hash;
    ReadLock lock(h);
    if(h->find(item))
        return true;
    else
        return false;
}

void HashType::clone(Value *out,const Value *in,bool deep)const{
    HashObject *p = new HashObject();
    Hash *h = get(const_cast<Value *>(in));
    
    WriteLock lock=WL(p->hash);
    
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

}
