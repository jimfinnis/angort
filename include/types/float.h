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
    }
    /// get the value of v as a float
    float get(Value *v);
    /// set the value to the given float
    void set(Value *v,float f);

    /// get a hash key
    virtual uint32_t getHash(Value *v);
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b);
    
    virtual int toInt(const Value *v) const;
    virtual long toLong(const Value *v) const;
    virtual float toFloat(const Value *v) const;
protected:
    virtual const char *toString(bool *allocated,const Value *v) const ;
};

}
#endif /* __FLOAT_H */
