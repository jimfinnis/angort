/**
 * @file native.h
 * @brief  Native functions
 *
 */

#ifndef __ANGORTNATIVE_H
#define __ANGORTNATIVE_H

namespace angort {

struct Property;


/// the type for a native function
class NativeType : public Type {
public:
    NativeType(){
        add("native","NATV");
    }
    
    virtual bool isCallable(){
        return true;
    }
    NativeFunc get(const Value *v);
    void set(Value *v,NativeFunc f);
};


/// the type for a native property object
class PropType : public Type {
public:
    PropType(){
        add("natprop","NATP");
    }
    
    Property *get(const Value *v);
    void set(Value *v,Property *f);
};

}
#endif /* __NATIVE_H */
