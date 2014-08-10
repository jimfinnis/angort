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
static int ctr=1;

void SymbolType::deleteAll(){
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
    v->clr();
    v->v.i = i;
    v->t = this;
}

const char *SymbolType::toString(bool *allocated,const Value *v) const {
    char buf[128];
    strncpy(buf,get(v),128);
    *allocated=true;
    return strdup(buf);
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
    //This does mean that the key "foo" will
    //not match the key `foo, but that's OK.
    return a->v.i == b->v.i;
}

}
