/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __HASHTYPE_H
#define __HASHTYPE_H

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
    
    /// create a new hash, throwing if it's not one
    Hash *set(Value *v);
    
    /// the default iterator for a hash is the key iterator
    virtual Iterator<Value *> *makeIterator(Value *v){
        return makeKeyIterator(v);
    }
    
    
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser);
    
    virtual void visitRefChildren(Value *v,ValueVisitor *visitor);
    
    virtual bool isIn(Value *v,Value *item);
    
    virtual void setValue(Value *coll,Value *k,Value *v);
    virtual void getValue(Value *coll,Value *k,Value *result);
    virtual int getCount(Value *coll);
    virtual void removeAndReturn(Value *coll,Value *k,Value *result);
};

    

#endif /* __HASHTYPE_H */
