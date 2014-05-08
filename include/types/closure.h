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
};

#endif /* __CLOSURE_H */
