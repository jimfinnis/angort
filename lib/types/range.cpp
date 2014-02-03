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

void RangeType::set(Value *v,int start,int end,int step){
    v->clr();
    v->t = this;
   
    Range *r = new Range();
    r->start = start;
    r->end = end;
    r->step = step;
    v->v.range = r;
    
    incRef(v);
}

struct RangeIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    Range *range; //!< the range we're iterating over
public:
    /// create a range iterator for a range
    RangeIterator(Range *r){
        range = r;
        /// increment the range's reference count
        range->incRefCt();
        
        /// initialise the current value to integer
        Types::tInteger->set(&v,range->start);
    }
    
    /// on destruction, delete the iterator
    ~RangeIterator(){
        range->decRefCt();
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
        return v.v.i > range->end;
    }
    
    /// return the current value
    virtual Value *current(){
        return &v;
    }
};

Iterator<Value *> *RangeType::makeValueIterator(Value *v){
    return new RangeIterator(v->v.range);
}

void RangeType::saveDataBlock(Serialiser *ser,const void *v){
    Range *r = (Range *)v;
    ser->file->writeInt(r->start);
    ser->file->writeInt(r->end);
    ser->file->writeInt(r->step);
}
void *RangeType::loadDataBlock(Serialiser *ser){
    Range *r = new Range();
    r->start = ser->file->readInt();
    r->end = ser->file->readInt();
    r->step = ser->file->readInt();
    return (void *)r;
}
