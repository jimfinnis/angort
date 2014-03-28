/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __CLOSURE_H
#define __CLOSURE_H

/// closure type
class ClosureType : public GCType {
public:
    ClosureType(){
        add("closure","CLOS");
    }
    void set(Value *v,struct Closure *c);
    
    /// serialise the closure data block
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the closure data block
    virtual void *loadDataBlock(Serialiser *ser);
    
    virtual void visitRefChildren(Value *v,ValueVisitor *visitor);
    
};

#endif /* __CLOSURE_H */
