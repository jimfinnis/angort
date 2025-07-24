/**
 * @file stdString.cpp
 * @brief  Brief description of file.
 *
 */

%doc
This library deals with strings, and tries to do so in a Unicode-friendly
way.
%doc

#include "angort.h"
#include <wchar.h>
#include <wctype.h>

using namespace angort;

namespace angort {
static char sbuf[1024];
void format(Value *out,Value *formatVal,ArrayList<Value> *items);

inline int wstrlen(const char *s){
    return mbstowcs(NULL,s,0);
}

}

%name string

%word stridx (haystack needle -- int) return index if haystack contains needle, else none
{
    const StringBuffer &needle = a->popString();
    const StringBuffer &haystack = a->popString();
    
    const char *p = strstr(haystack.get(),needle.get());
    if(!p)
        a->pushval()->clr();
    else
        a->pushInt(p-haystack.get());
}
%word istridx (haystack needle -- int) return index if haystack contains needle, else none
{
    const StringBuffer &needle = a->popString();
    const StringBuffer &haystack = a->popString();
    
    const char *p = strcasestr(haystack.get(),needle.get());
    if(!p)
        a->pushval()->clr();
    else
        a->pushInt(p-haystack.get());
}


%word chr (integer -- string) convert integer to ASCII char (as single-character string)
{
    Value *s = a->stack.peekptr();
    int i = s->toInt();
    char out[2];
    out[0]=i;
    out[1]=0;
    Types::tString->set(s,out);
    
}

%word asc (string -- integer) convert first character in string to ASCII integer
{
    Value *s = a->stack.peekptr();
    const StringBuffer& str = s->toString();
    Types::tInteger->set(s,*str.get());
}

%word lowercase (string -- string) lowercase a string 
{
    Value *v = a->stack.peekptr();
    const StringBuffer &s = v->toString();
    int len = wstrlen(s.get());
    wchar_t *buf = (wchar_t *)alloca((len+1)*sizeof(wchar_t));
    mbstowcs(buf,s.get(),len+1);
    for(int i=0;i<len;i++){
        buf[i]=towlower(buf[i]);
    }
    buf[len]=0;
    int reqd = wcstombs(NULL,buf,0);
    char *s2 = (char *)alloca(sizeof(char)*(reqd+1));
    wcstombs(s2,buf,reqd+1);
    Types::tString->set(v,s2);
}

%word uppercase (string -- string) uppercase a string 
{
    Value *v = a->stack.peekptr();
    const StringBuffer &s = v->toString();
    int len = wstrlen(s.get());
    wchar_t *buf = (wchar_t *)alloca((len+1)*sizeof(wchar_t));
    mbstowcs(buf,s.get(),len+1);
    
    for(int i=0;i<len;i++){
        buf[i]=towupper(buf[i]);
    }
    buf[len]=0;
    int reqd = wcstombs(NULL,buf,0);
    char *s2 = (char *)alloca(sizeof(char)*(reqd+1));
    wcstombs(s2,buf,reqd+1);
    Types::tString->set(v,s2);
}

%word format (list string -- string) string formatting
Format a string using a subset of printf semantics. The types supported
are: d, f, g, x, u, s. Precision and width are supported for
numeric types. The list contains the items to be substituted into the string.
{
    Value f,l;
    f.copy(a->popval());
    l.copy(a->popval());
    
    ArrayList<Value> *lst = Types::tList->get(&l); // format() will lock
    format(a->pushval(),&f,lst);
}

%word sx (v -- s) convert a value to hex string
{
    Value *s = a->stack.peekptr();
    int x = s->toInt();
    char buf[256];
    snprintf(buf,255,"%x",x);
    Types::tString->set(s,buf);
}
%word sf (v -- s) convert a float to a string with minimum sig figs
{
    Value *s = a->stack.peekptr();
    double x = s->toDouble();
    char buf[256];
    snprintf(buf,255,"%g",x);
    Types::tString->set(s,buf);
}

%word nsf (v n -- s) convert a float to a string with given sig figs
{
    int nsf = a->popInt();
    Value *s = a->stack.peekptr();
    double x = s->toDouble();
    char buf[256];
    char tmp[32];
    snprintf(tmp,32,"%%.%df",nsf);
    snprintf(buf,255,tmp,x);
    Types::tString->set(s,buf);
}


%word padleft (string padding -- string) insert spaces at left to pad out string
{
    wchar_t buf[1024];
    
    int padding = a->popInt();
    Value *v = a->stack.peekptr();
    const StringBuffer &s = v->toString();
    mbstowcs(buf,s.get(),1023);
    
    int len = wstrlen(s.get());
    if(len>=padding)
        return;
    if(len<0)
        return; // invalid length - do nothing
    
    wchar_t *newstr = (wchar_t *)malloc(sizeof(wchar_t)*(padding+1));
    int i;
    for(i=0;i<padding-len;i++)
        newstr[i]=L' ';
    wcscpy(newstr+i,buf);
    
    wcstombs(sbuf,newstr,1023);
    Types::tString->set(v,sbuf);
    free(newstr);
}

%word padright (string padding -- string) insert spaces at right to pad out string
{
    wchar_t buf[1024];
    
    
    int padding = a->popInt();
    Value *v = a->stack.peekptr();
    const StringBuffer &s = v->toString();
    mbstowcs(buf,s.get(),1023);
    
    
    int len = wstrlen(s.get());
    if(len>=padding)
        return;
    if(len<0)
        return; // invalid length - do nothing
    
    wchar_t *newstr = (wchar_t *)malloc(sizeof(wchar_t)*(padding+1));
    int i;
    wcscpy(newstr,buf);
    for(i=0;i<padding-len;i++)
        newstr[len+i]=L' ';
    newstr[padding]=0;
    
    wcstombs(sbuf,newstr,1023);
    Types::tString->set(v,sbuf);
    
    free(newstr);
}


%word trunc (string maxlen -- string) truncate a string if longer than maxlen
{
    wchar_t buf[1024];
    int maxlen = a->popInt();
    Value *v = a->stack.peekptr();
    const StringBuffer &s = v->toString();
    const char *src = s.get();
    int rv = mbstowcs(buf,src,1023);
    
    if(rv<0 || maxlen>1024 || maxlen<0 || wcslen(buf)<(unsigned int)maxlen)
        return;
    
    // might not actually be a string.
    buf[maxlen]=0;
    wcstombs(sbuf,buf,1023);
    Types::tString->set(v,sbuf);
}

inline int wordlen(const wchar_t * s){
    const wchar_t *p;
    for(p = s; *s && *s!=' ' && *s!='\t' && *s!='\n'; s++){}
    return(s-p);
}
%word wrap (string maxlen -- string) word wrap to max characters, inserting breaks. Tabs are one space!
{
    int maxlen = a->popInt();
    Value *v = a->stack.peekptr();
    const StringBuffer &sb = v->toString();
    const char *src = sb.get();
    int len = wstrlen(src);
    
    wchar_t *s = (wchar_t *)alloca((len+1)*sizeof(wchar_t));
    int rv = mbstowcs(s,src,len+1);
    if(rv<0){
        a->pushNone();
        return;
    }
    int llen=0;
    for(wchar_t *p = s;*p;p++,llen++){
        if(*p=='\n'){
            llen=0;
        } else if(*p==' ' || *p=='\t'){
            if(llen + wordlen(p+1) > maxlen){
                *p='\n';
                llen=0;
            }
        }
    }
    
    char *ss = (char *)malloc(len+1);
    rv=wcstombs(ss,s,len+1);
    Types::tString->set(v,ss);
}

%word split (string delim/max -- list) split a string on a single-character delimiter or with limit
delim can be either a delimiter or [delim,max] where max is the maximum
number of splits.
{
    Value *params[2];
    a->popParams(params,"sv");
    const StringBuffer& strb=params[0]->toString();
    const char *s = strdup(strb.get());
    char delim;
    int limit = -1;
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    WriteLock lock=WL(list);
    // param 1 is either a string or a list - if the latter, we split into delim and max splits
    
    if(params[1]->t == Types::tList){
        ArrayList<Value> *lst = Types::tList->get(params[1]);
        delim = lst->get(0)->toString().get()[0];
        limit = lst->get(1)->toInt();
    } else {
        delim = params[1]->toString().get()[0];
    }
    
    
    // must be the size of the longest string
    char *buf = (char *)malloc(strlen(s)+1);
    const char *p=s;
    const char *base=s;
    
    for(int splitct=0;;){
        if(*s==delim || !*s){
            memcpy(buf,p,s-p);
            buf[s-p]=0;
            Types::tString->set(list->append(),buf);
            if(!*s)break;//{free((void *)base);return;}
            if(++splitct == limit){
                // turn off the delimiter, avoiding more splits
                delim=0;
            }
            s++;
            p=s;
        } else
            s++;
    }
    free((void *)buf);
    free((void *)base);
}

%wordargs splitwhitespace s (str -- list) split string by whitespace
{
    int len = wstrlen(p0);
    wchar_t *buf = (wchar_t *)alloca((len+1)*sizeof(wchar_t));
    mbstowcs(buf,p0,len+1);
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    WriteLock lock=WL(list);
    
    wchar_t *s = buf;
    wchar_t *outbuf = (wchar_t *)alloca(len+1);
    char *outbufc = (char *)alloca(len*2+1); // a bit big, but there we go.
    wchar_t *outp = outbuf;
    
    while(*s){
        // skip initial whitespace
        while(*s && iswspace(*s))s++;
        if(!*s)break;
        // copy to end of word (or string)
        while(*s && !iswspace(*s)){
            *outp++ = *s++;
        }
        // terminate
        *outp++ = 0;
        // convert
        wcstombs(outbufc,outbuf,outp-outbuf);
        // append
        Types::tString->set(list->append(),outbufc);
        // reset
        outp=outbuf;
    }
}

%wordargs substr sii (str start count -- str) get a substring.
If count less than or equal to 0 calculate count from end, 
if start negative calculate start from end. See also "slice".
{
    int len = wstrlen(p0);
    if(p1<0)
        p1 = len+p1;
    if(p1>len || p1<0){
        a->pushString("");
    } else {
        wchar_t *s = (wchar_t *)alloca((len+1)*sizeof(wchar_t));
        int rv = mbstowcs(s,p0,len+1);
        if(rv<0){
            a->pushNone();
            return;
        }
        s+=p1;
        if(p2<=0){
            p2=len+p2-p1;
        }
        
        if(p2>len)p2=len;
        if(p2>0 && p1+p2<len)s[p2]=0;
        char *s2 = (char *)alloca(rv-p1+1);
        wcstombs(s2,s,rv-p1+1);
        a->pushString(s2);
    }
}        

%wordargs trim s (s --) trim surrounding whitespace
{
    int len = wstrlen(p0);
    wchar_t *buf = (wchar_t *)alloca((len+1)*sizeof(wchar_t));
    mbstowcs(buf,p0,len+1);
    
    
    wchar_t *s = buf;  // start of string
    while(*s && iswspace(*s))s++; // move first non-whitespace
    
    wchar_t *end = buf+len-1; // end of string (last char)
    // now move end of string back to first non whitespace
    if(iswspace(*end)){
        while(end>s && iswspace(*end))end--;
        end[1] = 0;
    }
    end++; // make room for terminator
    char *s2 = (char *)alloca(end-s+1);
    
    wcstombs(s2,s,end-s+1);
    a->pushString(s2);
}
