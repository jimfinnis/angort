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
    }
    /// get the value of v as a int
    int get(Value *v);
    /// set the value to the given int
    void set(Value *v,int f);
    
    /// get a hash key
    virtual uint32_t getHash(Value *v);
    
    /// are these two equal
    virtual bool equalForHashTable(Value *a,Value *b);
    
    
    virtual int toInt(const Value *v) const;
    virtual float toFloat(const Value *v) const;
protected:
    virtual const char *toString(bool *allocated,const Value *v) const ;
};

}
#endif /* __INT_H */
