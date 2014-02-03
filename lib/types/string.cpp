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

void StringType::set(Value *v,const char *s){
    v->clr();
    v->t = Types::tString;
    int len = strlen(s);
    strcpy(allocate(v,len+1),s);
}

void StringType::setPreAllocated(Value *v,const char *s){
    v->clr();
    v->t = Types::tString;
    v->v.s = (char *)s;
}

const char *StringType::toString(char *outBuf,int len,const Value *v) const {
    return getData(v);
}

float StringType::toFloat(const Value *v) const {
    return atof(getData(v));
}

int StringType::toInt(const Value *v) const {
    return atoi(getData(v));
}

void StringType::saveDataBlock(Serialiser *ser, const void *v){
    ser->file->writeString((const char *)v);
}
void *StringType::loadDataBlock(Serialiser *ser){
    // using some extra knowledge of how strings are saved and 
    // how blockallocated data works.
    int len = ser->file->read16();
    BlockAllocHeader *h = (BlockAllocHeader*)malloc(sizeof(BlockAllocHeader)+len);
    h->refct=0;
    ser->file->readBytes(h+1,len);
    return h;
}
