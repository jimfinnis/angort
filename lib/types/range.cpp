/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"
#include "file.h"
#include "ser.h"

template<> RangeType<int>::RangeType(){
    add("range","RANG");
}
template<> RangeType<float>::RangeType(){
    add("frange","FRNG");
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

// for hash keys, int ranges are equal if their members are equal
template<> uint32_t RangeType<int>::getHash(Value *v){
    return (uint32_t)(v->v.irange->start + v->v.irange->end*10000 + v->v.irange->step);
}
template<> bool RangeType<int>::equalForHashTable(Value *a,Value *b){
    if(a->t != b->t)return false;
    Range<int> *ra = a->v.irange;
    Range<int> *rb = b->v.irange;
    return ra->start == rb->start && ra->end == rb->end && ra->step == rb->step;
}

// for hash keys, float ranges are equal if they are the same range
template<> uint32_t RangeType<float>::getHash(Value *v){
    Range<float> *r = v->v.frange;
    return (uint32_t)(v->v.irange->start + v->v.irange->end*10000 + v->v.irange->step);
}
template<> bool RangeType<float>::equalForHashTable(Value *a,Value *b){
    return a==b;
}



struct IntRangeIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    Range<int> *range; //!< the range we're iterating over
public:
    /// create a range iterator for a range
    IntRangeIterator(Range<int> *r){
        range = r;
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
    FloatRangeIterator(Range<float> *r){
        range = r;
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
        return range->step < 0 ? (v.v.f < range->end) : (v.v.f > range->end);
    }
    
    /// return the current value
    virtual Value *current(){
        return &v;
    }
};

template <> Iterator<Value *> *RangeType<int>::makeValueIterator(Value *v){
    return new IntRangeIterator(v->v.irange);
}
template <> Iterator<Value *> *RangeType<float>::makeValueIterator(Value *v){
    return new FloatRangeIterator(v->v.frange);
}

template <> void RangeType<int>::saveDataBlock(Serialiser *ser,const void *v){
    Range<int> *r = (Range<int> *)v;
    ser->file->writeInt(r->start);
    ser->file->writeInt(r->end);
    ser->file->writeInt(r->step);
}
template <> void RangeType<float>::saveDataBlock(Serialiser *ser,const void *v){
    Range<float> *r = (Range<float> *)v;
    ser->file->writeFloat(r->start);
    ser->file->writeFloat(r->end);
    ser->file->writeFloat(r->step);
}
template <> void *RangeType<int>::loadDataBlock(Serialiser *ser){
    Range<int> *r = new Range<int>();
    r->start = ser->file->readInt();
    r->end = ser->file->readInt();
    r->step = ser->file->readInt();
    return (void *)r;
}
template <> void *RangeType<float>::loadDataBlock(Serialiser *ser){
    Range<float> *r = new Range<float>();
    r->start = ser->file->readFloat();
    r->end = ser->file->readFloat();
    r->step = ser->file->readFloat();
    return (void *)r;
}
