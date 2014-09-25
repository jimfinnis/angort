/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTRANGE_H
#define __ANGORTRANGE_H

namespace angort {

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
};

}
#endif /* __RANGE_H */
