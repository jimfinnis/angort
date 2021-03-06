/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTLONG_H
#define __ANGORTLONG_H

namespace angort {

/// long integer type
class LongType : public Type {
public:
    LongType(){
        add("long","INTL");
        makeNumber();
    }
    /// get the value of v as a int
    long get(Value *v) const;
    /// set the value to the given int
    void set(Value *v,long f) const;
    
    /// get a hash key
    virtual uint32_t getHash(Value *v) const;
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b) const;
    
    virtual void negate(Value *dest,Value *src) const ;
    virtual void absolute(Value *dest,Value *src) const;
    
    
    virtual int toInt(const Value *v) const;
    virtual long toLong(const Value *v) const;
    virtual float toFloat(const Value *v) const;
    virtual double toDouble(const Value *v) const;
    virtual void toSelf(Value *out,const Value *v) const;
    virtual void increment(Value *v,int step) const;

    virtual Iterator<Value *> *makeValueIterator(Value *v)const;

protected:
    virtual const char *toString(bool *allocated,const Value *v) const ;
};

}
#endif /* __INT_H */
