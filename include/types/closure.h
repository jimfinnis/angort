/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __CLOSURE_H
#define __CLOSURE_H

namespace angort {

/// closure type
class ClosureType : public GCType {
public:
    ClosureType(){
        add("closure");
    }
    virtual bool isCallable(){
        return true;
    }
    void set(Value *v,struct Closure *c);
};

}

#endif /* __CLOSURE_H */
