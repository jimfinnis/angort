/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __CODE_H
#define __CODE_H

/// pointer to a raw codeblock with no closure data
class CodeType : public Type { // note, they're not managed memory at all!
public:
    CodeType(){
        add("codeblock","CODE");
    }
    virtual bool isReference(){
        return true;
    }
    void set(Value *v,const struct CodeBlock *cb);
    
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser);
    
    virtual void saveValue(Serialiser *ser, Value *v);
    virtual void loadValue(Serialiser *ser, Value *v);
    
    
};


#endif /* __CODE_H */
