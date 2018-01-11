/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"

#define MAXSYMBOLLEN 32

namespace angort {
struct SymbolName {
    char s[MAXSYMBOLLEN];
};

/// symbols have their own private namespace
static StringMap<int> locations;
static ArrayList<SymbolName> strings(32);
int symbolCtr=1;

void SymbolType::deleteAll(){
    locations.clear();
    strings.clear();
}


const char *SymbolType::getunsafe(const Value *v) const {
    if(v->t == Types::tSymbol){
        return strings.getunsafe(v->v.i)->s;
    } else
        throw BadConversionException(v->t->name,name);
}

void SymbolType::get(const Value *v,char *buf128) const {
    if(v->t == Types::tSymbol){
        getString(v->v.i,buf128);
    } else
        throw BadConversionException(v->t->name,name);
}

bool SymbolType::exists(const char *s){
    return locations.find(s,NULL);
}


int SymbolType::getSymbol(const char *s){
    if(strlen(s)>MAXSYMBOLLEN)
        throw RUNT("ex$symbol","").set("symbol too long: %s",s);
    
    int n;
    strings.wlock();
    if(!locations.find(s,&n)){
        n = symbolCtr;
        locations.set(s,symbolCtr++);
        SymbolName *ss = strings.set(n);
        strcpy(ss->s,s);
    }
    strings.unlock();
    
    return n;
}

const char *SymbolType::getunsafeString(int id){
    return strings.getunsafe(id)->s;
}

void SymbolType::getString(int id,char *buf128){
    strings.lock();
    const char *s = strings.getunsafe(id)->s;
    strncpy(buf128,s,127);
    strings.unlock();
}

void SymbolType::set(Value *v,int i){
    v->clr();
    v->v.i = i;
    v->t = this;
}

const char *SymbolType::toString(bool *allocated,const Value *v) const {
    char buf[128];
    strings.lock();
    getString(v->v.i,buf);
    *allocated=true;
    strings.unlock();
    return strdup(buf);
}

int SymbolType::toInt(const Value *v) const {
    return (int)v->v.i;
}


uint32_t SymbolType::getHash(Value *v)const{
    // Fowler-Noll-Vo hash, variant 1a
    strings.lock();
    const unsigned char *s = (const unsigned char *)getunsafe(v);
    uint32_t h = 2166136261U;
    
    while(*s){
        h ^= *s++;
        h *= 16777619U;
    }
    strings.unlock();
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
