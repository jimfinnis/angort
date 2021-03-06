/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTSTRING_H
#define __ANGORTSTRING_H

namespace angort {

class StringType : public BlockAllocType {
public:
    StringType() {
        add("string","STRN");
        makeStringable();
    }
    virtual bool isReference()const{
        return true;
    }
    
    /// set the value to the given string, copying
    void set(Value *v,const char *s)const;
    /// set the value to the given string, copying, and taking
    /// the length (for certain kind of unicode strings)
    void setwithlen(Value *v,const char *s,int len) const;
    void setPreAllocated(Value *v,BlockAllocHeader *b)const;
    // just allocate and return ptr to head (will add 1 for terminator)
    char *setAllocateOnly(Value *v,int len) const;
    
    /// get length of string
    virtual int getCount(Value *coll)const;
    
    virtual float toFloat(const Value *v) const;
    virtual int toInt(const Value *v) const;
    virtual long toLong(const Value *v) const;
    virtual double toDouble(const Value *v) const;

    /// get a hash key
    virtual uint32_t getHash(Value *v)const;
    virtual void setValue(Value *coll,Value *k,Value *v)const;
    virtual void getValue(Value *coll,Value *k,Value *result)const;
    
    virtual void removeAndReturn(Value *coll,Value *k,Value *result)const{
        throw RUNT("ex$string","cannot remove from string");
    }
    
    /// return the index of item is in the collection (uses the same
    /// equality test as hash keys).If not present, returns -1.
    virtual int getIndexOfContainedItem(Value *v,Value *item)const;
    virtual bool contains(Value *v,Value *item) const;
    
    virtual Iterator<Value *> *makeValueIterator(Value *v)const;
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b)const;
    virtual void slice(Value *out,Value *coll,int start,int end)const;
    virtual void slice_dep(Value *out,Value *coll,int start,int len)const;
    virtual void clone(Value *out,const Value *in,bool deep=false)const;

    virtual void toSelf(Value *out,const Value *v) const;
protected:
    virtual const char *toString(bool *allocated,const Value *v) const;
};

}
#endif /* __STRING_H */
