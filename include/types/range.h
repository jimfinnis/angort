/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __RANGE_H
#define __RANGE_H

#include "angort.h"

template <class T> struct Range : public GarbageCollected {
    T start,end,step;
    virtual Iterator<class Value *> *makeValueIterator();
    
    /// the key iterator of a range is the same as the value iterator
    virtual Iterator<class Value *> *makeKeyIterator(){
        return makeValueIterator();
    }
};

template <class T> class RangeType : public GCType {
public:
    void set(Value *v,T start,T end,T step);
    RangeType();

    /// get a hash key
    virtual uint32_t getHash(Value *v);
    
    virtual bool isIn(Value *v,Value *item);
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b);
    
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser);
    
};

#endif /* __RANGE_H */
