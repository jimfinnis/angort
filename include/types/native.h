/**
 * @file native.h
 * @brief  Native functions
 *
 */

#ifndef __NATIVE_H
#define __NATIVE_H

#include "../plugins.h"

namespace angort {

struct Property;


/// the type for a native function
class NativeType : public Type {
public:
    NativeType(){
        add("native");
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
        add("natprop");
    }
    
    Property *get(const Value *v);
    void set(Value *v,Property *f);
};

/// the type for a plugin function
class PluginFuncType : public Type {
public:
    PluginFuncType(){
        add("plugin");
    }
    
    virtual bool isCallable(){
        return true;
    }
    PluginFunc *get(const Value *v);
    void set(Value *v,PluginFunc *f);
};

}
#endif /* __NATIVE_H */
