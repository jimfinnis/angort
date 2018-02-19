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
    virtual Iterator<class Value *> *makeValueIterator()const;
    
    /// the key iterator of a range is the same as the value iterator
    virtual Iterator<class Value *> *makeKeyIterator()const{
        return makeValueIterator();
    }
    
//    virtual ~Range(){
//        printf("%lu Delete range at %p\n",pthread_self(),this);
//    }
    Range() : GarbageCollected("range"){
        start = 0;
        end = 10;
        step = 1;
    }
    Range(const Range<T>& r) : GarbageCollected("range"){
        start = r.start;
        end = r.end;
        step = r.step;
    }
};

template <class T> class RangeType : public GCType {
public:
    void set(Value *v,T start,T end,T step)const;
    void set(Value *v,Range<T> *r)const;
    RangeType();

    /// get a hash key
    virtual uint32_t getHash(Value *v)const;
    
    virtual bool isIn(Value *v,Value *item)const;
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b)const;
    
    virtual void clone(Value *out,const Value *in,bool deep=false)const;
};

}
#endif /* __RANGE_H */
