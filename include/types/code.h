/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTCODE_H
#define __ANGORTCODE_H

namespace angort {

/// pointer to a raw codeblock with no closure data
class CodeType : public Type { // note, they're not managed memory at all!
public:
    CodeType(){
        add("codeblock","CODE");
    }
    virtual bool isReference(){
        return true;
    }
    virtual bool isCallable(){
        return true;
    }
    void set(Value *v,const struct CodeBlock *cb);
};

}
#endif /* __CODE_H */
