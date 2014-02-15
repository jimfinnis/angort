/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */


#include "angort.h"
#include "file.h"
#include "ser.h"


float FloatType::get(Value *v){
    if(v->t == Types::tInteger)
        return (float)v->v.i;
    else if(v->t == Types::tFloat)
        return (float)v->v.f;
    else
        throw BadConversionException(v->t->name,name);
}


void FloatType::set(Value *v,float f) {
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

const char * FloatType::toString(char *outBuf,int len,const Value *v) const{
    snprintf(outBuf,len,"%f",v->v.f);
    return outBuf;
}

uint32_t FloatType::getHash(Value *v){
    // there are much, much better float
    // hashes out there - but they're very
    // complex and slow.
    return (uint32_t)v->v.f;
}

bool FloatType::equalForHashTable(Value *a,Value *b){
    return a->toFloat() == b->toFloat();
}


void FloatType::saveValue(Serialiser *ser, Value *v){
    ser->file->writeFloat(get(v));
}
void FloatType::loadValue(Serialiser *ser, Value *v){
    set(v,ser->file->readFloat());
}
