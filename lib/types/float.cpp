/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */


#include "angort.h"

namespace angort {


float FloatType::get(Value *v) const{
    if(v->t == Types::tInteger)
        return (float)v->v.i;
    else if(v->t == Types::tDouble)
        return (float)v->v.df;
    else if(v->t == Types::tFloat)
        return (float)v->v.f;
    else
        throw BadConversionException(v->t->name,name);
}


void FloatType::set(Value *v,float f) const {
    v->clr();
    v->t = Types::tFloat;
    v->v.f=f;
}

int FloatType::toInt(const Value *v) const {
    return (int)v->v.f;
}

float FloatType::toFloat(const Value *v) const {
    return v->v.f;
}

double FloatType::toDouble(const Value *v) const {
    return v->v.f;
}

long FloatType::toLong(const Value *v) const {
    return (long)v->v.f;
}

const char *FloatType::toString(bool *allocated,const Value *v) const {
    char buf[128];
    snprintf(buf,128,"%f",v->v.f);
    *allocated=true;
    return strdup(buf);
}

uint32_t FloatType::getHash(Value *v) const{
    // there are much, much better float
    // hashes out there - but they're very
    // complex and slow.
    return (uint32_t)v->v.f;
}

bool FloatType::equalForHashTable(Value *a,Value *b) const {
    return a->toFloat() == b->toFloat();
}

void FloatType::toSelf(Value *out,const Value *v) const {
    set(out,v->toFloat());
}



}
