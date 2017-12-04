/**
 * @file double.h
 * @brief  Brief description of file.
 *
 */

#ifndef __ANGORTDOUBLE_H
#define __ANGORTDOUBLE_H

#include "../types.h"

namespace angort {

/// double floating point type object, permitting
/// conversions

class DoubleType : public Type {
public:
    DoubleType(){
        add("double","DFLT");
        makeNumber();
        strcpy(formatString,"%f");
    }
    /// get the value of v as a float
    double get(Value *v)const;
    /// set the value to the given float
    void set(Value *v,double f)const;

    /// get a hash key
    virtual uint32_t getHash(Value *v)const;
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b)const;
    
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


#endif /* __DOUBLE_H */
