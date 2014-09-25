/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTHASHTYPE_H
#define __ANGORTHASHTYPE_H

namespace angort {

class Hash;

struct HashObject: public GarbageCollected {
    Hash *hash;
    
    virtual Iterator<Value *> *makeKeyIterator();
    virtual Iterator<Value *> *makeValueIterator();
    HashObject();
    ~HashObject();
};

class HashType: public GCType {
public:
    HashType(){
        add("hash","HASH");
    }
    
    /// get the hash, throwing if it's not one
    Hash *get(Value *v);
    
    /// create a new hash
    Hash *set(Value *v);
    
    /// the default iterator for a hash is the key iterator
    virtual Iterator<Value *> *makeIterator(Value *v){
        return makeKeyIterator(v);
    }
    
    
    virtual bool isIn(Value *v,Value *item);
    
    virtual void setValue(Value *coll,Value *k,Value *v);
    virtual void getValue(Value *coll,Value *k,Value *result);
    virtual int getCount(Value *coll);
    virtual const char *toString(bool *allocated, const Value *v) const;
    virtual void removeAndReturn(Value *coll,Value *k,Value *result);
    virtual void slice(Value *out,Value *coll,int start,int len){
        throw RUNT("cannot get slice of hash");
    }
    virtual void clone(Value *out,const Value *in,bool deep=false);
};

}    

#endif /* __HASHTYPE_H */
