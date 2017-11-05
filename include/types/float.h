/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTFLOAT_H
#define __ANGORTFLOAT_H

#include "../types.h"

namespace angort {

/// floating point type object, permitting
/// conversions

class FloatType : public Type {
public:
    FloatType(){
        add("float","FLOT");
        strcpy(formatString,"%f");
        makeNumber();
    }
    /// get the value of v as a float
    float get(Value *v) const ;
    /// set the value to the given float
    void set(Value *v,float f)const ;

    /// get a hash key
    virtual uint32_t getHash(Value *v) const;
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b) const;
    
    virtual void negate(Value *dest,Value *src) const;
    virtual void absolute(Value *dest,Value *src) const;
    
    virtual int toInt(const Value *v) const;
    virtual long toLong(const Value *v) const;
    virtual float toFloat(const Value *v) const;
    virtual double toDouble(const Value *v) const;
    virtual void toSelf(Value *out,const Value *v) const;
    virtual void increment(Value *v,int step) const;
    
    char formatString[64]; // used for toString()
    
protected:
    virtual const char *toString(bool *allocated,const Value *v) const ;
};

}
#endif /* __FLOAT_H */
