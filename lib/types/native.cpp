/**
 * @file native.cpp
 * @brief Native function type
 *
 */


#include "angort.h"

namespace angort {


NativeFunc NativeType::get(const Value *v) {
    if(v->t == this)
        return v->v.native;
    else
        throw BadConversionException(v->t->name,name);
}

void NativeType::set(Value *v,NativeFunc f) {
    v->clr();
    v->v.native=f;
    v->t = this;
}

Property *PropType::get(const Value *v) {
    if(v->t == this)
        return v->v.property;
    else
        throw BadConversionException(v->t->name,name);
}

void PropType::set(Value *v,Property *f) {
    v->clr();
    v->v.property=f;
    v->t = this;
}
    
    
}
