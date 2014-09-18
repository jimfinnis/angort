/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __GC_H
#define __GC_H

namespace angort {

struct Value;

#define dprintf printf
//#define dprintf if(0)printf

typedef uint16_t refct_t; //!< reference count - make sure it's unsigned


/// a garbage collected entity. This is not a subclass of Value or Type, but a Value may
/// reference one via the v.gc field.

class GarbageCollected {
    static int globalCount;
public:
    
    /// set refct to zero, add to cycle detection system
    GarbageCollected();
    /// remove from cycle detection system
    virtual ~GarbageCollected();
    
    /// return the total number of refcounted objects
    static int getGlobalCount(){
        return globalCount;
    }
    
    
    /// the reference count
    refct_t refct;
    
    /// the reference count used by the cycle detector. The name
    /// comes from the original doc (see CycleDetector).
    refct_t gc_refs;
    
    /// pointer for maintaining container list
    GarbageCollected *next; 
    /// pointer for maintaining container list
    GarbageCollected *prev;
    
    /// increment the refct, throwing an exception if it wraps
    void incRefCt(){
        refct++;
        dprintf("++ incrementing count for %p, now %d\n",this,refct);
        if(refct==0)
            throw Exception("ref count too large");
    }

    /// decrement the reference count returning true if it became zero
    bool decRefCt(){
        if(refct<=0)
            throw Exception().set("ERROR - already deleted: %p!",this);
        --refct;
        dprintf("-- decrementing count for %p, now %d\n",this,refct);
        return refct==0;
    }
    
    
    /// many GC objects are containers for references to other objects - return a reference
    /// to an iterator iterating over containing these, and you won't need to subclass the methods 
    /// \todo rewrite docs for iterators?
    virtual Iterator<class Value *> *makeKeyIterator(){return NULL;}
    /// many GC objects are containers for references to other objects - return a reference
    /// to an iterator iterating over containing these, and you won't need to subclass the methods 
    /// below! This is the values iterator.
    virtual Iterator<class Value *> *makeValueIterator(){return NULL;}
    
    /// deletion prepwork - see CycleDetector::detect(). This should go through any GC objects,
    /// and if the gc_refs field is 0xffff, should clear them (i.e. any objects which were not traced)
    /// Extend for C++ properties which are garbage-collectable.
    virtual void clearZombieReferences(){}
    
    /// decrement gc_refct in all collectable entities I point to.
    /// Extend for C++ properties which are garbage-collectable
    virtual void decReferentsCycleRefCounts(){}
    
    /// trace all collectable entities reachable from this object.
    /// If their gc_refs is zero (i.e. still in the mainlist) move
    /// to the list passed in (the newlist). See CycleDetector for
    /// more details; it's also something best learned by examples!
    /// Extend for C++ properties which are garbage-collectable,
    /// and aren't accessed by iterator.
    virtual void traceAndMove(class CycleDetector *cycle){}
    
    /// run the cycle detector to do a major garbage detect
    static void gc();
};

}
#endif /* __GC_H */
