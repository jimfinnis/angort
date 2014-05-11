/**
 * @file native.h
 * @brief  Native functions
 *
 */

#ifndef __NATIVE_H
#define __NATIVE_H

#include "plugins.h"

/// the type for a native function
class NativeType : public Type {
public:
    NativeType(){
        add("native","NATV");
    }
    
    virtual bool isCallable(){
        return true;
    }
    PluginFunc *get(const Value *v);
    void set(Value *v,PluginFunc *f);
};



#endif /* __NATIVE_H */
