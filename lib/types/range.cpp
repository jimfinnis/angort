/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"

#include <math.h>

namespace angort {

template<> RangeType<int>::RangeType()  {
    flags |= TF_ITERABLE;
    add("range","RANI");
}
template<> RangeType<float>::RangeType() {
    flags |= TF_ITERABLE;
    add("frange","RANF");
}

template <> void RangeType<float>::set(Value *v, float start,float end,float step){
    v->clr();
    v->t = this;
   
    Range<float> *r = new Range<float>();
    r->start = start;
    r->end = end;
    r->step = step;
    v->v.frange = r;
    
    incRef(v);
}
template <> void RangeType<int>::set(Value *v, int start,int end,int step){
    v->clr();
    v->t = this;
   
    Range<int> *r = new Range<int>();
    r->start = start;
    r->end = end;
    r->step = step;
    v->v.irange = r;
    
    incRef(v);
}

template<> bool RangeType<int>::isIn(Value *v,Value *item)const{
    Range<int> *r = v->v.irange;
    int i = item->toInt();
    if(i<r->start || i>=r->end)
        return false;
    return ((i-r->start)%r->step)==0;
}


// for hash keys, int ranges are equal if their members are equal
template<> uint32_t RangeType<int>::getHash(Value *v)const{
    return (uint32_t)(v->v.irange->start + v->v.irange->end*10000 + v->v.irange->step);
}
template<> bool RangeType<int>::equalForHashTable(Value *a,Value *b)const{
    if(a->t != b->t)return false;
    Range<int> *ra = a->v.irange;
    Range<int> *rb = b->v.irange;
    return ra->start == rb->start && ra->end == rb->end && ra->step == rb->step;
}

// for hash keys, float ranges are equal if they are the same range
template<> uint32_t RangeType<float>::getHash(Value *v)const{
    return (uint32_t)(v->v.irange->start + v->v.irange->end*10000 + v->v.irange->step);
}
template<> bool RangeType<float>::equalForHashTable(Value *a,Value *b)const{
    return a==b;
}



struct IntRangeIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    Range<int> *range; //!< the range we're iterating over
public:
    /// create a range iterator for a range. The constness is a mess.
    IntRangeIterator(const Range<int> *r){
        range = (Range<int> *)r;
        /// increment the range's reference count
        range->incRefCt();
        Types::tInteger->set(&v,range->start);
        
    }
    
    /// on destruction, delete the iterator
    ~IntRangeIterator(){
        if(range->decRefCt())
            delete range;
    }
    
    /// set the current value to the first item
    virtual void first(){
        v.v.i = range->start;
    }
    /// set the current value to the next item
    virtual void next(){
        v.v.i += range->step;
    }
    /// return true if we're out of bounds
    virtual bool isDone() const{
        return range->step < 0 ? (v.v.i <= range->end) : (v.v.i >= range->end);
    }
    
    /// return the current value
    virtual Value *current(){
        return &v;
    }
};


struct FloatRangeIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    Range<float> *range; //!< the range we're iterating over
public:
    /// create a range iterator for a range
    FloatRangeIterator(const Range<float> *r){
        range = (Range<float>*)r;
        /// increment the range's reference count
        range->incRefCt();
        Types::tFloat->set(&v,range->start);
        
    }
    
    /// on destruction, delete the iterator
    ~FloatRangeIterator(){
        if(range->decRefCt())
            delete range;
    }
    
    /// set the current value to the first item
    virtual void first(){
        v.v.f = range->start;
    }
    /// set the current value to the next item
    virtual void next(){
        v.v.f += range->step;
    }
    /// return true if we're out of bounds
    virtual bool isDone() const{
        return range->step < 0 ? (v.v.f <= range->end) : (v.v.f >= range->end);
    }
    
    /// return the current value
    virtual Value *current(){
        return &v;
    }
};

template <> Iterator<Value *> *Range<int>::makeValueIterator()const{
    return new IntRangeIterator(this);
}
template <> Iterator<Value *> *Range<float>::makeValueIterator()const{
    return new FloatRangeIterator(this);
}

template<> bool RangeType<float>::isIn(Value *v,Value *item)const{
    throw RUNT("ex$range","cannot determine membership of float range");
}

}
