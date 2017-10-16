/**
 * @file nsid.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"

namespace angort {

    

int NSIDType::get(Value *v) const {
    if(v->t != this)
        throw BadConversionException(v->t->name,name);
    return v->v.i;
}


void NSIDType::set(Value *v,int i) const {
    v->clr();
    v->t = Types::tNSID;
    v->v.i=i;
}

void NSIDType::toSelf(Value *out,const Value *v) const {
    set(out,v->v.i);
}


const char *NSIDType::toString(bool *allocated,const Value *v) const {
    char buf[128];
    snprintf(buf,128,"NSID:%d",v->v.i);
    *allocated=true;
    return strdup(buf);
}

uint32_t NSIDType::getHash(Value *v) const {
    return (uint32_t)v->v.i;
}

bool NSIDType::equalForHashTable(Value *a,Value *b) const {
    return a->toInt() == b->toInt();
}

}
