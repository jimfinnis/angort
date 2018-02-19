/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTGC_H
#define __ANGORTGC_H

#include "lock.h"

namespace angort {

struct Value;

//#define dprintf printf
#define dprintf if(0)printf

typedef uint16_t refct_t; //!< reference count - make sure it's unsigned


/// a garbage collected entity. This is not a subclass of Value or Type, but a Value may
/// reference one via the v.gc field.

class GarbageCollected : public Lockable {
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
    
    /// true if we've been found in a GC cycle and we're being deleted
    bool inCycle;
    
    /// the reference count used by the cycle detector. The name
    /// comes from the original doc (see CycleDetector).
    refct_t gc_refs;
    
    /// pointer for maintaining container list
    GarbageCollected *next; 
    /// pointer for maintaining container list
    GarbageCollected *prev;
    
    
    /// increment the refct, throwing an exception if it wraps
    void incRefCt(){
        WriteLock lock = WL(this);
        refct++;
        dprintf("++ incrementing count for %p, now %d\n",this,refct);
        if(refct==0)
            throw RUNT(EX_REFS,"ref count too large");
    }

    /// decrement the reference count returning true if it became zero
    bool decRefCt(){
        WriteLock lock = WL(this);
        if(refct<=0)
            throw RUNT(EX_REFS,"").set("ERROR - already deleted: %p!",this);
        --refct;
        dprintf("-- decrementing count for %p, now %d\n",this,refct);
        return refct==0;
    }
    
    /// called from inside the cycle delete code to safely wipe
    /// the object's contents, leaving NO REFERENCES to GC objects.
    /// This avoids recursive deletion in cycle detection.
    /// Typically, all Value contents will have wipeIfInGCCycle() called.
    virtual void wipeContents() {}
    
    
    /// many GC objects are containers for references to other objects - return a reference
    /// to an iterator iterating over containing these, and you won't need to subclass the methods 
    /// \todo rewrite docs for iterators?
    virtual Iterator<class Value *> *makeKeyIterator()const {
        return NULL;
    }
    /// many GC objects are containers for references to other objects - return a reference
    /// to an iterator iterating over containing these, and you won't need to subclass the methods 
    /// below! This is the values iterator.
    virtual Iterator<class Value *> *makeValueIterator() const {
        return NULL;
    }
    
    /// this is the default function for making an iterator for use for cycle detection - override
    /// this to return NULL if the type is NOT iterating over true values inside, and bad things would
    /// happen if we were to try to cycle detect in here (such is where iterating over a file type fakes up string objects)
    virtual Iterator<class Value *> *makeGCKeyIterator(){
        return makeKeyIterator();
    }
    
    /// this is the default function for making an iterator for use for cycle detection - override
    /// this to return NULL if the type is NOT iterating over true values inside, and bad things would
    /// happen if we were to try to cycle detect in here (such is where iterating over a file type fakes up string objects)
    virtual Iterator<class Value *> *makeGCValueIterator(){
        return makeValueIterator();
    }
    
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
