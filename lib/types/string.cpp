/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"

void StringType::set(Value *v,const char *s){
    int len = strlen(s);
    strcpy(allocate(v,len+1,this),s);
}

int StringType::getCount(Value *v){
    return strlen(v->v.s);
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

uint32_t StringType::getHash(Value *v){
    // Fowler-Noll-Vo hash, variant 1a
    const unsigned char *s = (const unsigned char *)getData(v);
    uint32_t h = 2166136261U;
    
    while(*s){
        h ^= *s++;
        h *= 16777619U;
    }
    return h;
}

bool StringType::equalForHashTable(Value *a,Value *b){
    if(a->t != b->t)return false;
    return !strcmp(getData(a),getData(b));
}

