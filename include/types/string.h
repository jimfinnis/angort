/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __STRING_H
#define __STRING_H

class StringType : public BlockAllocType {
public:
    StringType() {
        add("string","STRN");
    }
    virtual bool isReference(){
        return true;
    }
    /// set the value to the given string, copying
    void set(Value *v,const char *s);
    /// pass this a block allocated by setStringPreAllocated() --- it's used
    /// by string concatenation etc. MUST BE in the right format, i.e. starting
    /// with a refcount.
    void setPreAllocated(Value *v,const char *s);
    
    /// get length of string
    virtual int getCount(Value *v);
    
    /// convert to a string - just returns the string, length is ignored
    virtual const char *toString(char *outBuf,int len,const Value *v) const;
    virtual float toFloat(const Value *v) const;
    virtual int toInt(const Value *v) const;

    /// get a hash key
    virtual uint32_t getHash(Value *v);
    
    virtual void setValue(Value *coll,Value *k,Value *v);
    
    virtual void getValue(Value *coll,Value *k,Value *result);
    
    virtual void removeAndReturn(Value *coll,Value *k,Value *result){
        throw RUNT("cannot remove from string");
    }
    
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b);
};


#endif /* __STRING_H */
