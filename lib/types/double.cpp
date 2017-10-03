/**
 * @file double.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"

namespace angort {


double DoubleType::get(Value *v)const{
    if(v->t == Types::tInteger)
        return (double)v->v.i;
    else if(v->t == Types::tFloat)
        return (double)v->v.f;
    else if(v->t == Types::tDouble)
        return (double)v->v.df;
    else
        throw BadConversionException(v->t->name,name);
}


void DoubleType::set(Value *v,double f) const {
    v->clr();
    v->t = Types::tDouble;
    v->v.df=f;
}

int DoubleType::toInt(const Value *v) const {
    return (int)v->v.df;
}
double DoubleType::toDouble(const Value *v) const {
    return v->v.df;
}
float DoubleType::toFloat(const Value *v) const {
    return (float)v->v.df;
}
long DoubleType::toLong(const Value *v) const {
    return (long)v->v.df;
}

const char *DoubleType::toString(bool *allocated,const Value *v) const {
    char buf[128];
    snprintf(buf,128,formatString,v->v.df);
    *allocated=true;
    return strdup(buf);
}

uint32_t DoubleType::getHash(Value *v)const{
    // there are much, much better float
    // hashes out there - but they're very
    // complex and slow.
    return (uint32_t)v->v.df;
}

bool DoubleType::equalForHashTable(Value *a,Value *b)const{
    return a->toDouble() == b->toDouble();
}

void DoubleType::toSelf(Value *out,const Value *v) const {
    set(out,v->toDouble());
}

void DoubleType::absolute(Value *dest,Value *src) const {
    double v = src->toDouble();
    set(dest,v<0?-v:v);
}

void DoubleType::negate(Value *dest,Value *src) const {
    set(dest,-src->toDouble());
}


}
