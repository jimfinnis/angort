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
public:
    T start,end,step;
};

template <class T> class RangeType : public GCType {
public:
    void set(Value *v,T start,T end,T step);
    RangeType();

    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser);
    
    
    virtual Iterator<Value *> *makeValueIterator(Value *v);
};

#endif /* __RANGE_H */
