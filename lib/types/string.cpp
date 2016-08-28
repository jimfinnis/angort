/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"
#include <wchar.h>

namespace angort {

class StringIterator : public Iterator<Value *> {
private:
    Value v; // result
    Value string; // string we're going over
    StringBuffer buf; // string buffer
    const wchar_t *wstr; // the string (as wide)
    char tmp[2];
    int idx,len;
public:
    const char *p,*str;
    StringIterator(const Value *v);
    virtual ~StringIterator(){}
    virtual void first();
    virtual void next();
    virtual bool isDone() const;
    virtual Value *current();
};


StringIterator::StringIterator(const Value *s){
    string.copy(s);
    buf.set(s);
    wstr=buf.getWide();
    idx=0;
    len=wcslen(wstr);
}

void StringIterator::first(){
    idx=0;
}
void StringIterator::next(){
    idx++;
}

bool StringIterator::isDone() const {
    return idx>=len;
}
Value *StringIterator::current(){
    int n = wctomb(tmp,wstr[idx]);
    tmp[n]=0;
    Types::tString->set(&v,tmp);
    return &v;
}





void StringType::set(Value *v,const char *s){
    int len = strlen(s);
    char *dest = allocate(v,len+1,this);
    strcpy(dest,s);
}

int StringType::getCount(Value *v){
    const char *s = getData(v);
    return mbstowcs(NULL,s,0);
}


void StringType::setPreAllocated(Value *v,BlockAllocHeader *b){
    v->clr();
    v->t = Types::tString;
    v->v.block = b;
}

const char *StringType::toString(bool *allocated,const Value *v) const {
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

void StringType::setValue(Value *coll,Value *k,Value *v){
    char *s = (char *)getData(coll);
    int idx = k->toInt();
    s[idx]=v->toString().get()[0];
}
void StringType::getValue(Value *coll,Value *k,Value *result){
    const char *s = (char *)getData(coll);
    
    int idx = k->toInt();
    char out[2];
    out[1]=0;
    out[0]=s[idx];
    set(result,out);
}

void StringType::slice(Value *out,Value *coll,int start,int len){
    const StringBuffer &b = StringBuffer(coll);
    wchar_t *s = b.getWideBuffer(); // allocates memory
    
    int slen = wcslen(s);
    if(start<0)start=0;
    if(len<0)len=slen;
    
    if(start>=slen)
        set(out,"");
    else {
        if(len > (slen-start))
            len = slen-start;
        if(len<=0)
            set(out,"");
        else
        {
            wchar_t *outbuf = s+start;
            outbuf[len]=0;
            
            // and convert back.
            
            mbstate_t state;
            memset(&state,0,sizeof(state));
            
            const wchar_t *p = outbuf;
            int mblen = wcsrtombs(NULL,&p,0,&state);
            char *smb = allocate(out,mblen+1,this);
            p = outbuf;
            wcsrtombs(smb,&p,mblen+1,&state); // +1 for the null
        }
    }
    
    
    free(s); // free the buffer
}

void StringType::clone(Value *out,const Value *in,bool deep){
    const char *s = getData(in);
    // note - will work for UTF-8, because gives memory size,
    // not character count
    int len = strlen(s); 
    
    BlockAllocHeader *h = (BlockAllocHeader *)malloc(len+1+sizeof(BlockAllocHeader));
    h->refct=1;
    memcpy((char *)(h+1),s,len+1); // and the null too!
    
    out->v.block = h;
    out->t = this;
}


Iterator<Value *> *StringType::makeValueIterator(Value *v){
    return new StringIterator(v);
}

}
