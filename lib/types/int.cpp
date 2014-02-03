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

int IntegerType::get(Value *v){
    if(v->t == Types::tInteger)
        return v->v.i;
    else if(v->t == Types::tFloat)
        return (int)v->v.f;
    else
        throw BadConversionException(v->t->name,name);
}


void IntegerType::set(Value *v,int i){
    v->clr();
    v->t = Types::tInteger;
    v->v.i=i;
}

int IntegerType::toInt(const Value *v) const {
    return v->v.i;
}

float IntegerType::toFloat(const Value *v) const {
    return (float)v->v.i;
}

const char * IntegerType::toString(char *outBuf,int len,const Value *v) const {
    snprintf(outBuf,len,"%d",v->v.i);
    return outBuf;
}

void IntegerType::saveValue(Serialiser *ser, Value *v){
    ser->file->writeInt(get(v));
}
void IntegerType::loadValue(Serialiser *ser, Value *v){
    set(v,ser->file->readInt());
}
