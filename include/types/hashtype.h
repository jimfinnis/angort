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
    
    virtual Iterator<Value *> *makeKeyIterator()const;
    virtual Iterator<Value *> *makeValueIterator()const;
    HashObject();
    ~HashObject();
};

class HashType: public GCType {
public:
    HashType(){
        flags |= TF_ITERABLE;
        add("hash","HASH");
    }
    
    /// get the hash, throwing if it's not one
    Hash *get(Value *v)const;
    
    /// create a new hash
    Hash *set(Value *v)const;
    
    /// the default iterator for a hash is the key iterator
    virtual Iterator<Value *> *makeIterator(Value *v) const{
        return makeKeyIterator(v);
    }
    
    /// set a value to an existing hash
    virtual void set(Value *v,HashObject *lo)const;
    
    virtual bool isIn(Value *v,Value *item)const;
    
    virtual void setValue(Value *coll,Value *k,Value *v)const;
    virtual void getValue(Value *coll,Value *k,Value *result)const;
    virtual int getCount(Value *coll)const;
    virtual void removeAndReturn(Value *coll,Value *k,Value *result)const;
    virtual void slice(Value *out,Value *coll,int start,int len)const{
        throw RUNT("ex$nocol","cannot get slice of hash");
    }
    virtual void clone(Value *out,const Value *in,bool deep=false)const;
};

}    

#endif /* __HASHTYPE_H */
