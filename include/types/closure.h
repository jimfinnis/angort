/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTCLOSURE_H
#define __ANGORTCLOSURE_H

namespace angort {

/// closure object, which  wraps the closure
/// block and the codeblock. It also wraps the
/// closure map, so there can be two different kinds:
/// 1) an outer function, which has both a table and a map because it
///    both stores data and refers to it;
/// 2) an inner function, which has only a map, because it only refers
///    to data in other closure blocks and doesn't have one of its own.

class Closure : public GarbageCollected
{
public:
    const CodeBlock *cb;
    Value *block; //!< the variables I own
    Value **map; //!< pointers to both the above and other's variables I look at
    Closure **blocksUsed; //!< the blocks the map uses, so I can deref them
    Closure *parent; //!< link to parent closure (which created me)
    struct Instruction *ip; //!< used when routines yield
    
    /// constructing a closure does almost nothing, because the object may have
    /// to be inserted into various bits of Angort first. Once this is done,
    /// init() is called to complete the construction.
    Closure(Closure *parent);
    
    // this will create a table, and also a map if
    // required. To do this, it will scan Angort's return stack, which it will get access
    // to with getCallingInstance().
    void init(const CodeBlock *_cb);
    virtual ~Closure();
    Iterator<class Value *> *makeValueIterator();
    
    void show(const char *s);
    
    virtual void clearZombieReferences();
    virtual void decReferentsCycleRefCounts();
    virtual void traceAndMove(class CycleDetector *cycle);

};


/// closure type
class ClosureType : public GCType {
    
public:
    ClosureType(){
        add("closure","CLOS");
        
    }
    
    
    virtual bool isCallable(){
        return true;
    }
    void set(Value *v, Closure *c);
};

}

#endif /* __CLOSURE_H */
