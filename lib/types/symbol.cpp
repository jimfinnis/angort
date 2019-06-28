/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"

#define MAXSYMBOLLEN 128

namespace angort {
struct SymbolName {
    char s[MAXSYMBOLLEN];
};

/// symbols have their own private namespace
static StringMap<int> locations;
static ArrayList<SymbolName> strings(32);
int symbolCtr=1;

void SymbolType::deleteAll(){
    WriteLock l=WL(Types::tSymbol);
    locations.clear();
    strings.clear();
}


const char *SymbolType::get(const Value *v) const {
    
    if(v->t == Types::tSymbol){
        return strings.get(v->v.i)->s;
    } else
        throw BadConversionException(v->t->name,name);
          
}

bool SymbolType::exists(const char *s){
    return locations.find(s);
}


int SymbolType::getSymbol(const char *s){
    if(strlen(s)>MAXSYMBOLLEN)
        throw RUNT("ex$symbol","").set("symbol too long: %s",s);
    
    int n;
    
    // unpleasant - see
    // https://groups.google.com/forum/#!topic/comp.programming.threads/QsJI57oQZKc
    WriteLock l=WL(Types::tSymbol);
    
    if(locations.find(s)){
        n=locations.found();
    } else {
        n = symbolCtr;
        locations.set(s,symbolCtr++);
        SymbolName *ss = strings.set(n);
        strcpy(ss->s,s);
    }
    
    return n;
}

const char *SymbolType::getString(int id){
    return strings.get(id)->s;
}

void SymbolType::set(Value *v,int i){
    v->clr();
    v->v.i = i;
    v->t = this;
}

const char *SymbolType::toString(bool *allocated,const Value *v) const {
    ReadLock l(this);
    char buf[128];
    strncpy(buf,get(v),128);
    *allocated=true;
    return strdup(buf);
}

int SymbolType::toInt(const Value *v) const {
    return (int)v->v.i;
}


uint32_t SymbolType::getHash(Value *v)const{
    ReadLock l(this);
    // Fowler-Noll-Vo hash, variant 1a
    const unsigned char *s = (const unsigned char *)get(v);
    uint32_t h = 2166136261U;
    
    while(*s){
        h ^= *s++;
        h *= 16777619U;
    }
    return h;
}

bool SymbolType::equalForHashTable(Value *a,Value *b)const{
    //This does mean that the key "foo" will
    //not match the key `foo, but that's OK.
    return a->v.i == b->v.i;
}

int getSymbolID(const char *s){
    return Types::tSymbol->getSymbol(s);
}

}
