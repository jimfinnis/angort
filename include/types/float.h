/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __FLOAT_H
#define __FLOAT_H

#include "../types.h"

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
    virtual float toFloat(const Value *v) const;
    virtual const char *toString(char *outBuf,int len,const Value *v) const;

    virtual void saveValue(Serialiser *ser, Value *v);
    virtual void loadValue(Serialiser *ser, Value *v);
};

#endif /* __FLOAT_H */
