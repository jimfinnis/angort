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

struct Range : public GarbageCollected {
    int start,end,step;
};

class RangeType : public GCType {
public:
    RangeType(){
        add("range","RANG");
    }
    void set(Value *v,int start,int end,int step);

    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser);
    
    
protected:
    virtual Iterator<Value *> *makeValueIterator(Value *v);
};

#endif /* __RANGE_H */
