/**
 * @file stdString.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"
#include <wchar.h>

%name string

static char sbuf[1024];

inline int wstrlen(const char *s){
    return mbstowcs(NULL,s,0);
}

%word stridx (haystack needle -- int) return index if h contains n, else none
{
    const StringBuffer &needle = a->popString();
    const StringBuffer &haystack = a->popString();
    
    int rv;
    const char *p = strstr(haystack.get(),needle.get());
    if(!p)
        a->pushval()->clr();
    else
        a->pushInt(p-haystack.get());
}
%word istridx (haystack needle -- int) return index if h contains n, else none
{
    const StringBuffer &needle = a->popString();
    const StringBuffer &haystack = a->popString();
    
    int rv;
    const char *p = strcasestr(haystack.get(),needle.get());
    if(!p)
        a->pushval()->clr();
    else
        a->pushInt(p-haystack.get());
}

%word substr (start len str -- sub) extract and copy out a substring
{
    const StringBuffer &s = a->popString();
    const char *str = s.get();
    strcpy(sbuf,str); // make sure it gets copied
    
    int len = a->popInt();
    int start = a->popInt();
    
    
    if(start>=strlen(sbuf)){
        a->pushString("");
    } else {
        Value *s = a->pushval();
        
        if(len>strlen(sbuf+start))
            len = strlen(sbuf);
    
        char *out = Types::tString->allocate(s,len+1,Types::tString);
        memcpy(out,sbuf+start,len);
        out[start+len]=0;
    }
}

%word toint (string -- int) string to integer
{
    Value *v = a->stack.peekptr();
    Types::tInteger->set(v,v->toInt());
}
        
%word tofloat (string -- float) string to float
{
    Value *v = a->stack.peekptr();
    Types::tFloat->set(v,v->toFloat());
}
        

%word chr (integer -- string) convert integer to ASCII
{
    Value *s = a->stack.peekptr();
    int i = s->toInt();
    char out[2];
    out[0]=i;
    out[1]=0;
    Types::tString->set(s,out);
    
}

%word asc (string -- integer) convert ASCII char to integer
{
    Value *s = a->stack.peekptr();
    const StringBuffer& str = s->toString();
    Types::tInteger->set(s,*str.get());
}

%word format (list string -- string) string formatting
{
    void format(Value *out,Value *formatVal,ArrayList<Value> *items);
    
    Value *f = a->popval();
    Value *l = a->popval();
    
    format(a->pushval(),f,Types::tList->get(l));
          
    
}

%word padleft (string padding -- string) insert spaces at left to pad out string
{
    wchar_t buf[1024];
    
    int padding = a->popInt();
    Value *v = a->stack.peekptr();
    const StringBuffer &s = v->toString();
    mbstowcs(buf,s.get(),1024);
    
    int len = wstrlen(s.get());
    if(len>=padding)
        return;
    
    wchar_t *newstr = (wchar_t *)malloc(sizeof(wchar_t)*(padding+1));
    int i;
    for(i=0;i<padding-len;i++)
        newstr[i]=L' ';
    wcscpy(newstr+i,buf);
    wcstombs(sbuf,newstr,1024);
    
    Types::tString->set(v,sbuf);
    free(newstr);
}

%word padright (string padding -- string) insert spaces at right to pad out string
{
    wchar_t buf[1024];
    
    int padding = a->popInt();
    Value *v = a->stack.peekptr();
    const StringBuffer &s = v->toString();
    mbstowcs(buf,s.get(),1024);
    
    
    int len = wstrlen(s.get());
    if(len>=padding)
        return;
    
    wchar_t *newstr = (wchar_t *)malloc(sizeof(wchar_t)*(padding+1));
    int i;
    wcscpy(newstr,buf);
    for(i=0;i<padding-len;i++)
        newstr[len+i]=L' ';
    newstr[padding]=0;
    wcstombs(sbuf,newstr,1024);
    
    Types::tString->set(v,sbuf);
    free(newstr);
}


%word truncstr (string maxlen -- string) truncate a string if required
{
    wchar_t buf[1024];
    
    int maxlen = a->popInt();
    Value *v = a->stack.peekptr();
    const StringBuffer &s = v->toString();
    mbstowcs(buf,s.get(),1024);
    
    if(maxlen>1024 || maxlen<0 || wcslen(buf)<maxlen)
        return;
    
    // might not actually be a string.
    buf[maxlen]=0;
    wcstombs(sbuf,buf,1024);
    Types::tString->set(v,sbuf);
}

