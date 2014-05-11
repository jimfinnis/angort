/**
 * @file native.cpp
 * @brief Native function type
 *
 */


#include "angort.h"

PluginFunc *NativeType::get(const Value *v) {
    if(v->t == this)
        return v->v.native;
    else
        throw BadConversionException(v->t->name,name);
}

void NativeType::set(Value *v,PluginFunc *f) {
    v->clr();
    v->v.native=f;
    v->t = this;
}
    
