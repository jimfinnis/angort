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
    virtual int index() const {
        return idx;
    }
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





void StringType::set(Value *v,const char *s)const{
    int len = strlen(s);
    char *dest = allocate(v,len+1,this);
    strcpy(dest,s);
}

char *StringType::setAllocateOnly(Value *v,int len)const{
    return allocate(v,len+1,this);
}

void StringType::setwithlen(Value *v,const char *s,int len)const{
    char *dest = allocate(v,len+1,this);
    memcpy(dest,s,len);
    dest[len]=0;
}

int StringType::getCount(Value *v)const{
    const char *s = getData(v);
    return mbstowcs(NULL,s,0);
}


void StringType::setPreAllocated(Value *v,BlockAllocHeader *b)const{
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

double StringType::toDouble(const Value *v) const {
    return atof(getData(v));
}

int StringType::toInt(const Value *v) const {
    return atoi(getData(v));
}

long StringType::toLong(const Value *v) const {
    return atol(getData(v));
}

uint32_t StringType::getHash(Value *v)const{
    // Fowler-Noll-Vo hash, variant 1a
    const unsigned char *s = (const unsigned char *)getData(v);
    uint32_t h = 2166136261U;
    
    while(*s){
        h ^= *s++;
        h *= 16777619U;
    }
    return h;
}

bool StringType::equalForHashTable(Value *a,Value *b)const{
    if(a->t != b->t)return false;
    return !strcmp(getData(a),getData(b));
}

void StringType::setValue(Value *coll,Value *k,Value *v)const{
    char *s = (char *)getData(coll);
    int idx = k->toInt();
    s[idx]=v->toString().get()[0];
}
void StringType::getValue(Value *coll,Value *k,Value *result)const{
    const char *s = (char *)getData(coll);
    
    int idx = k->toInt();
    char out[2];
    out[1]=0;
    out[0]=s[idx];
    set(result,out);
}

/// deprecated version
void StringType::slice_dep(Value *out,Value *coll,int start,int len)const{
    const StringBuffer &b = StringBuffer(coll);
    wchar_t *s = b.getWideBuffer(); // allocates memory
    
    int slen = wcslen(s);
    if(start<0)start=slen+start;
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

/// good version
void StringType::slice(Value *out,Value *coll,int startin,int endin)const{
    const StringBuffer &b = StringBuffer(coll);
    wchar_t *s = b.getWideBuffer(); // allocates memory
    
    int start,end;
    int slen = wcslen(s);
    bool ok = getSliceEndpoints(&start,&end,slen,startin,endin);
    
    
    if(ok){
        // getWideBuffer will have allocated some data
        // we can change, so we work out where the slice is
        // in this buffer and terminate it.
        s[end]=0;
        wchar_t *slicebuf = s+start;
        // slicebuf is now a null-terminated slice inside the wide
        // buffer.
        
        // and now convert back to mbs
        
        mbstate_t state;
        memset(&state,0,sizeof(state));
        
        const wchar_t *p = slicebuf;
        int mblen = wcsrtombs(NULL,&p,0,&state);
        char *smb = allocate(out,mblen+1,this);
        p = slicebuf;
        wcsrtombs(smb,&p,mblen+1,&state); // +1 for the null
    } else {
        set(out,"");
    }
    
    free(s); // free the wide buffer
}

void StringType::clone(Value *out,const Value *in,bool deep)const{
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

void StringType::toSelf(Value *out,const Value *v) const {
    set(out,v->toString().get());
}


Iterator<Value *> *StringType::makeValueIterator(Value *v)const{
    return new StringIterator(v);
}

int StringType::getIndexOfContainedItem(Value *v,Value *item)const {
    const StringBuffer &b = StringBuffer(v);
    const wchar_t *haystack = b.getWideBuffer();
    const StringBuffer &b2 = StringBuffer(item);
    const wchar_t *needle = b2.getWideBuffer();
    
    const wchar_t *res = wcsstr(haystack,needle);
    int out = res ? res-haystack: -1;
    free((void *)needle);
    free((void *)haystack);
    return out;
}

bool StringType::contains(Value *v,Value *item) const {
    const StringBuffer &b = StringBuffer(v);
    const wchar_t *haystack = b.getWideBuffer();
    const StringBuffer &b2 = StringBuffer(item);
    const wchar_t *needle = b2.getWideBuffer();
    
    const wchar_t *res = wcsstr(haystack,needle);
    free((void *)needle);
    free((void *)haystack);
    return res!=NULL;
}


}
