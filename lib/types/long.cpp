/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */


#include "angort.h"

namespace angort {

    

long LongType::get(Value *v){
    /*
    if(v->t == Types::tInteger)
        return v->v.i;
    else if(v->t == Types::tFloat)
        return (int)v->v.f;
    else
       throw BadConversionException(v->t->name,name);
     */
    if(v->t != this)
        RUNT("expected a long");
    return v->v.l;
}


void LongType::set(Value *v,long i){
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

long LongType::toLong(const Value *v) const {
    return v->v.l;
}

const char *LongType::toString(bool *allocated,const Value *v) const {
    char buf[128];
    snprintf(buf,128,"%ld",v->v.l);
    *allocated=true;
    return strdup(buf);
}

uint32_t LongType::getHash(Value *v){
    // crude.
    return (uint32_t)v->v.l;
}

bool LongType::equalForHashTable(Value *a,Value *b){
    return get(a) == get(b);
}


}
