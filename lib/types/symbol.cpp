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

#define MAXSYMBOLLEN 32

struct SymbolName {
    char s[MAXSYMBOLLEN];
};

/// symbols have their own private namespace
static StringMap<int> locations;
static ArrayList<SymbolName> strings(16);
static int ctr=1;

const char *SymbolType::get(const Value *v) const {
    if(v->t == Types::tSymbol)
        return strings.get(v->v.i)->s;
    else
        throw BadConversionException(v->t->name,name);
          
}

int SymbolType::getSymbol(const char *s){
    if(strlen(s)>MAXSYMBOLLEN)
        throw RUNT("").set("symbol too long: %s",s);
    
    int n;
    if(locations.find(s)){
        n=locations.found();
    } else {
        n = ctr;
        locations.set(s,ctr++);
        SymbolName *ss = strings.set(n);
        strcpy(ss->s,s);
    }
    
    return n;
}

const char *SymbolType::getString(int id){
    return strings.get(id)->s;
}

void SymbolType::set(Value *v,int i){
    int n;
    v->clr();
    v->v.i = i;
    v->t = this;
}

const char * SymbolType::toString(char *outBuf,int len,const Value *v) const {
    strncpy(outBuf,get(v),len);
    return outBuf;
}

uint32_t SymbolType::getHash(Value *v){
    // Fowler-Noll-Vo hash, variant 1a
    const unsigned char *s = (const unsigned char *)get(v);
    uint32_t h = 2166136261U;
    
    while(*s){
        h ^= *s++;
        h *= 16777619U;
    }
    return h;
}

bool SymbolType::equalForHashTable(Value *a,Value *b){
    // This does mean that the key "foo" will
    // not match the key `foo, but that's OK.
    return a->v.i = b->v.i;
}


void SymbolType::saveValue(Serialiser *ser, Value *v){
    ser->file->writeString(get(v));
}
void SymbolType::loadValue(Serialiser *ser, Value *v){
    char buf[MAXSYMBOLLEN];
    int i = getSymbol(ser->file->readString(buf,MAXSYMBOLLEN));
    set(v,i);
}