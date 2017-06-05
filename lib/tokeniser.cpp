#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "tokeniser.h"

namespace angort {

void Tokeniser::dprintf(const char *s,...)
{
    if(trace)
    {
        char buf[256];
        
        va_list args;
        va_start(args,s);
        
        vsprintf(buf,s,args);
        printf("TOKENISER> %s\n",buf);
    }
}

bool Tokeniser::getnextstring(char *out){
    if(getnext()!=stringtoken)
    {
        seterror();
        return false;
    }
    strcpy(out,val.s);
    return true;
}

bool Tokeniser::getnextident(char *out){
    if(getnext()!=identtoken)
    {
        seterror();
        return false;
    }
    strcpy(out,val.s);
    return true;
}

bool Tokeniser::getnextidentorkeyword(char *out){
    keywordsOff=true;
    bool rv = getnextident(out);
    keywordsOff=false;
    return rv;
}


void Tokeniser::init()
{
    for(int i=0;i<128;i++)
        chartable[i]=-100;
    tokens = NULL;
    handler = NULL;
    digraphct = 0;
    trace=0;
}

void Tokeniser::reset(const char *buf,const char *_end)
{
    start = buf;
    current = buf;
    if(_end)
        end = _end;
    else
        end = buf+strlen(buf);
    
    error=false;
    line =0;
    keywordsOff=false;
    curtype = -1000;
}

void Tokeniser::skipahead(char c)
{
    while(*current!=c){
        current++;
        if(current==end)
        {
            seterror();
            return;
        }
    }
    current++; // to skip the char
}

const char *Tokeniser::skipspace(const char *p)
{
    while(*p && (*p==' ' || *p=='\t' || *p==0xd || *p==0xa))
    {
        if(*p == 0x0a)line++;
        p++;
    }
    return p;
}    


void Tokeniser::rewind()
{
    current = previous;
    curtype = prevtype;
    val = prevval;
    line = prevline;
}

const char *Tokeniser::restofline(){
    static char buf[1024];
    int i=0;
    
    const char *p = current;
    while(*p && *p != 0x0a && i<1023){
        buf[i++] = *p++;
    }
    buf[i]=0;
    current=p;
    return buf;
}

int Tokeniser::getnext()
{
    previous = current;
    prevtype = curtype;
    prevval = val;
    prevline = line;
    
    const char *p = (char *)current;
    
    if(curtype==endtoken)return curtype;
    if(p>=end)
    {
        curtype = endtoken;
        dprintf("got END");
        return curtype;
    }
    
loop:
    /// skip whitespace
    p=skipspace(p);
    
    /// skip comments
    if(commentlinesequencelen)
    {
        if(!strncmp(p,commentlinesequence,commentlinesequencelen))
        {
            // skip to end of line
            while(*p!=0xd && *p!=0x0a)p++;
            /// and following white space
            p=skipspace(p);
            goto loop;
        }
    }
    
    /// is it possibly a digraph?
    
    for(int i=0;i<digraphct;i++){
        if(p[0] == digraphtable[i].c1 && p[1] == digraphtable[i].c2){
            current=p+2;
            return digraphtable[i].token;
        }
    }
    
    /// is it a special char?
    
    if(chartable[(unsigned char )*p]>-100)
    {
        
        curtype=chartable[(unsigned char )*p];
        
        // special case ignoring for '.' AND '-' before a number
        bool possibleNumChar = *p == '.' || *p=='-';
        if(!possibleNumChar || !isdigit(p[1])){
            
            // if the token type is -ve, return the character as the token type
            if(curtype == -1)
                curtype = *p;
            
            dprintf("got char - %c",*p,curtype);
            val.s[0]=*p++;
            val.s[1]=0;
            current=p;
            
            
            return curtype;
        }
    }
    const char *b;
    
    int len;
    if(*p=='"' || *p=='\'')
    {
        char buf[1024];
        char *strout = buf;
        
        char q = *p;
        p++;
        while(*p!=q)
        {
            if(*p == 0x0a)line++;
            if(*p == 0){
                seterror();
                return -1;
            }
            
            // string escape handing
            if(*p == '\\'){
                if(isdigit(p[1])){
                    // octal
                    int d = (*++p - '0')*8*8;
                    if(!*++p){
                        seterror();return -1;
                    }
                    d += (*p - '0')*8;
                    if(!*++p){
                        seterror();return -1;
                    }
                    d += (*p - '0');
                    *strout++ = (char)d;
                } else {
                    switch(*++p){
                    case 'n':                    *strout++='\n';                    break;
                    case 't':                    *strout++='\t';                    break;
                    case 'r':                    *strout++='\r';                    break;
                    case '\\':                   *strout++='\\';                    break;
                    case '\'':                   *strout++='\'';                    break;
                    case '\"':                   *strout++='\"';                    break;
                    default:
                        seterror();
                        return -1;
                    }
                }
                p++;
            }else
                  *strout++ = *p++;
        }
        *strout++=0;
        
        len = strout-buf;
        
        memcpy(val.s,buf,len);
        current = p+1;
        curtype=stringtoken;
        
        dprintf("got delimited string");
        return curtype;
    }
    else if(*p)
    {
        b=p;
        // get char into buffer while:
        // - not whitespace and
        // - not special character token or digit preceded by . or -
        while((*p && *p!=' ' && *p!='\t' && *p!=0x0a &&
               *p!=0x0d && 
               (chartable[(unsigned char)*p]<-1)) || 
              (*p && isdigit(p[1]) && (*p=='.'||*p=='-')))p++;
        len = p-b;
        memcpy(val.s,b,len);
        val.s[len]=0;
        current=p;
        
        // check for number
        
        
        if(isdigit(val.s[0]) || val.s[0]=='-'){ // starts with a digit, must be a number. Hex would be 0ffh etc.
            bool gotpoint;
            bool isLong = false;
            char *exponent; // exponent (ptr to 'e') if present
            
            // first, look at the last character to see if it's a long
            if(tolower(val.s[len-1]=='l')){
                // if so, set the flag and chop off that char.
                val.s[--len]=0;
                isLong=true;
            }
            // is there a point? (Indicates float)
            gotpoint = strchr(val.s,'.')!=NULL;
            // or an exponent? (Also indicates float - and keep the ptr)
            exponent = strchr(val.s,'e');
            
            // get the (possibly new) end char for the base.
            char endchar = val.s[len-1];
            if(isalpha(endchar)){
                if(gotpoint && !exponent){ // can't have a base char on a float/double
                    seterror();
                    return -1; 
                }else{
                    // evaluate
                    long x;
                    switch(tolower(endchar)){
                    case 'd':x=atol(val.s);break;
                    case 'h':
                    case 'x':x=strtol(val.s,NULL,16);break;
                    case 'b':x=strtol(val.s,NULL,2);break;
                    case 'o':x=strtol(val.s,NULL,8);break;
                    default:
                        curtype=identtoken; // return as ident if bad char!
                        return curtype;
                    }
                    val.i=x;
                    curtype = isLong ? longtoken : inttoken;
                    return curtype;
                }
            } else if(gotpoint || exponent){
                // this might have an E in it, for exponential notation
                double v;
                if(exponent){
                    *exponent = 0;
                    double mant = atof(val.s);
                    double expn = atof(exponent+1);
                    *exponent = 'e'; // best put it back
                    v = pow(10.0,expn)*mant;
                } else {
                    v = atof(val.s);
                }
                    
                if(isLong){
                    val.df = v;
                    curtype = doubletoken;
                } else {
                    val.f = v;
                    curtype = floattoken;
                }                    
            } else {
                val.i= atol(val.s);
                curtype = isLong ? longtoken : inttoken;
            }
            return curtype;
        }
        
        int w=-1;
        if(!keywordsOff)
            w = findkeyword(val.s);
        if(w>=0)
        {
            dprintf("got keyword %s",val.s);
            curtype = w;
        }
        else
        {
            dprintf("got ident - %s",val.s);
            curtype = identtoken;
        }
        return curtype;
    }
    curtype = endtoken;
    return curtype;
}

int Tokeniser::findkeyword(const char *s)
{
    TokenRegistry *k;
    
    for(k=tokens;k->word;k++)
    {
        // ignore specials
        if(*(k->word) != '*' && !strcmp(s,k->word))
            return k->token;
    }
    
    // not found.
    
    return -1;
}

void Tokeniser::settokens(TokenRegistry *k)
{
    tokens = k;
    
    // and set the specials
    
    for(k=tokens;k->word;k++) {
        if(*(k->word) == '*') {
            switch(k->word[1])
            {
            case 'i': identtoken = k->token; break;
            case 's': stringtoken = k->token; break;
            case 'n': inttoken = k->token; break;
            case 'f': floattoken = k->token; break;
            case 'D': doubletoken = k->token; break;
            case 'L': longtoken = k->token; break;
            case 'e': endtoken = k->token;break;
            case 'c':
                {
                    dprintf("registering %c",k->word[2]);
                    char c = k->word[2];
                    chartable[c&0x7f]=k->token;
                }
                break;
            case 'd':
                digraphtable[digraphct].c1 = k->word[2];
                digraphtable[digraphct].c2 = k->word[3];
                digraphtable[digraphct++].token = k->token;
            default: // ignore badly formed entries
                break;
            }
        }
    }
}




}
