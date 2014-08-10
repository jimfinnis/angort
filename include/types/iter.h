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

namespace angort {

/// a garbage-collectable object wrapped around an iterator of any sort.
class IteratorObject : public GarbageCollected {
public:
    /// another object will create the iterator, which we delete.
    IteratorObject(Iterator<Value *> *iter,Value *src);
    
    ~IteratorObject();

    Iterator<Value *> *iterator;
    /// we have to have this because of the way iterators and loops work;
    /// our loops go "leaveifdone next ... getcurrent.. "
    /// so we need to stash the actual current before we call next.
    Value *current;
    
    /// and here we keep a copy of the thing we're iterating over
    Value *iterable;
};


class IteratorType : public GCType {
public:
    IteratorType(){
        add("iterator");
    }
    
    void set(Value *v,Value *src,Iterator<Value *> *iter);
    Iterator<Value *> *get(Value *v);
};

}
#endif /* __ITER_H */
