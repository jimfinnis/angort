/**
 * @file plugobj.cpp
 * @brief  Plugin object wrapper type
 *
 */

#include "angort.h"

PluginObjectWrapper *PluginObjectType::get(const Value *v){
    if(v->t == this)
        return v->v.plobj;
    else
        throw BadConversionException(v->t->name,name);
}

void PluginObjectType::set(Value *v,PluginObjectWrapper *obj){
    v->clr();
    v->v.plobj=obj;
    incRef(v);
    v->t=this;
}
