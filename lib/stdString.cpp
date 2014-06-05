/**
 * @file stdString.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"

%name string


%word stridx (haystack needle -- int) return index if h contains n, else none
{
    char nbuf[1024];
    char hbuf[1024];
    const char *needle = a->popval()->toString(nbuf,1024);
    const char *haystack = a->popval()->toString(hbuf,1024);
    
    int rv;
    const char *p = strstr(haystack,needle);
    if(!p)
        a->pushval()->clr();
    else
        a->pushInt(p-haystack);
}
%word istridx (haystack needle -- int) return index if h contains n, else none
{
    char nbuf[1024];
    char hbuf[1024];
    const char *needle = a->popval()->toString(nbuf,1024);
    const char *haystack = a->popval()->toString(hbuf,1024);
    
    int rv;
    const char *p = strcasestr(haystack,needle);
    if(!p)
        a->pushval()->clr();
    else
        a->pushInt(p-haystack);
}

%word substr (start len str -- sub) extract and copy out a substring
{
    char buf[1024];
    Value *s = a->popval();
    const char *str = s->toString(buf,1024);
    strcpy(buf,str); // make sure it gets copied
    
    int len = a->popInt();
    int start = a->popInt();
    
    
    if(start>=strlen(buf)){
        a->pushString("");
    } else {
        Value *s = a->pushval();
        if(len>strlen(buf+start))
            len = strlen(buf);
    
        char *out = Types::tString->allocate(s,len+1,Types::tString);
        memcpy(out,buf+start,len);
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
    Types::tFloat->set(v,v->toInt());
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
    char buf[32];
    Value *s = a->stack.peekptr();
    const char *str = s->toString(buf,32);
    Types::tInteger->set(s,str[0]);
}
