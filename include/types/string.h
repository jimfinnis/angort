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
    
    /// convert to a string - just returns the string, length is ignored
    virtual const char *toString(char *outBuf,int len,const Value *v) const;
    virtual float toFloat(const Value *v) const;
    virtual int toInt(const Value *v) const;

    /// get a hash key
    virtual uint32_t getHash(Value *v);
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b);
    
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser);
    

};


#endif /* __STRING_H */
