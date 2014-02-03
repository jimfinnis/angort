/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __INT_H
#define __INT_H

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
    
    virtual int toInt(const Value *v) const;
    virtual float toFloat(const Value *v) const;
    virtual const char *toString(char *outBuf,int len,const Value *v) const;

    virtual void saveValue(Serialiser *ser, Value *v);
    virtual void loadValue(Serialiser *ser, Value *v);

};


#endif /* __INT_H */
