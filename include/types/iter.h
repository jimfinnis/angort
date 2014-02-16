/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ITER_H
#define __ITER_H

#include "angort.h"

/// a garbage-collectable object wrapped around an iterator of any sort.
class IteratorObject : public GarbageCollected {
public:
    /// another object will create the iterator, which we delete.
    IteratorObject(Iterator<Value *> *iter);
    
    ~IteratorObject();

    Iterator<Value *> *iterator;
    /// we have to have this because of the way iterators and loops work;
    /// our loops go "leaveifdone next ... getcurrent.. "
    /// so we need to stash the actual current before we call next.
    Value *current;
};


class IteratorType : public GCType {
public:
    IteratorType(){
        add("iterator","ITER");
    }
    
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v){
        throw WTF;
    }
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser){
        throw WTF;
    }
    virtual void saveValue(Serialiser *ser, Value *v){
        throw WTF;
    }
    virtual void loadValue(Serialiser *ser, Value *v){
        throw WTF;
    }
    
    void set(Value *v,Iterator<Value *> *iter);
    Iterator<Value *> *get(Value *v);
};

#endif /* __ITER_H */
