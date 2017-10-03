/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */


#include "angort.h"

namespace angort {

    

long LongType::get(Value *v) const{
    /*
    if(v->t == Types::tInteger)
        return v->v.i;
    else if(v->t == Types::tFloat)
        return (int)v->v.f;
    else
       throw BadConversionException(v->t->name,name);
     */
    if(v->t != this)
        RUNT("ex$long","expected a long");
    return v->v.l;
}


void LongType::set(Value *v,long i)const{
    v->clr();
    v->t = this;
    v->v.l=i;
}

int LongType::toInt(const Value *v) const {
    return (int)v->v.l;
}

float LongType::toFloat(const Value *v) const {
    return (float)v->v.l;
}

double LongType::toDouble(const Value *v) const {
    return (double)v->v.l;
}

long LongType::toLong(const Value *v) const {
    return v->v.l;
}

const char *LongType::toString(bool *allocated,const Value *v) const {
    char buf[128];
    snprintf(buf,128,"%ld",v->v.l);
    *allocated=true;
    return strdup(buf);
}

uint32_t LongType::getHash(Value *v) const{
    // crude.
    return (uint32_t)v->v.l;
}

bool LongType::equalForHashTable(Value *a,Value *b) const{
    return get(a) == get(b);
}

void LongType::toSelf(Value *out,const Value *v) const {
    set(out,v->toLong());
}

void LongType::absolute(Value *dest,Value *src) const {
    long v = src->toLong();
    set(dest,v<0?-v:v);
}

void LongType::negate(Value *dest,Value *src) const {
    set(dest,-src->toLong());
}


}
