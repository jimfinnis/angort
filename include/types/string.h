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
        add("string");
    }
    virtual bool isReference(){
        return true;
    }
    /// set the value to the given string, copying
    void set(Value *v,const char *s);
    void setPreAllocated(Value *v,BlockAllocHeader *b);
    
    /// get length of string
    virtual int getCount(Value *v);
    
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
    virtual void slice(Value *out,Value *coll,int start,int len);
    
    virtual void clone(Value *out,const Value *in,bool deep=false);

protected:
    virtual const char *toString(bool *allocated,const Value *v) const;
};


#endif /* __STRING_H */
