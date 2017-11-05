/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTINT_H
#define __ANGORTINT_H

namespace angort {

/// integer type
class IntegerType : public Type {
public:
    IntegerType(){
        add("integer","INTG");
        makeNumber();
    }
    /// get the value of v as a int
    int get(Value *v) const;
    /// set the value to the given int
    void set(Value *v,int f) const;
    
    /// get a hash key
    virtual uint32_t getHash(Value *v) const;
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b) const;
    
    virtual void negate(Value *dest,Value *src) const;
    virtual void absolute(Value *dest,Value *src) const;
    
    virtual int toInt(const Value *v) const;
    virtual long toLong(const Value *v) const;
    virtual double toDouble(const Value *v) const;
    virtual float toFloat(const Value *v) const;
    virtual void toSelf(Value *out,const Value *v) const;
    virtual void increment(Value *v,int step) const;
protected:
    virtual const char *toString(bool *allocated,const Value *v) const ;
};

}
#endif /* __INT_H */
