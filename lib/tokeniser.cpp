#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>

#include "tokeniser.h"

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
    return true;
}


void Tokeniser::init()
{
    for(int i=0;i<128;i++)
        chartable[i]=-100;
    tokens = NULL;
    handler = NULL;
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
    
    const char *p = current;
    
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
    
    /// is it a special char?
    
    if(chartable[*p]>-100)
    {
        
        curtype=chartable[*p];
        
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
                switch(*++p){
                case 'n':                    *strout++='\n';                    break;
                case 't':                    *strout++='\t';                    break;
                case 'r':                    *strout++='\r';                    break;
                case '\\':                   *strout++='\\';                    break;
                case '\'':                   *strout++='\'';                    break;
                case '\"':                   *strout++='\"';                    break;
                default:seterror();return -1;
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
        while(*p && *p!=' ' && *p!='\t' && *p!=0x0a &&
              *p!=0x0d && 
              (chartable[*p]<-1 || isdigit(p[1]) && (*p=='.'||*p=='-')))p++;
        len = p-b;
        memcpy(val.s,b,len);
        val.s[len]=0;
        current=p;
        
        // check for number
        
        bool gotPoint = false;
        
        for(int i=0;i<len;i++)
        {
            if(val.s[i] == '.')
                gotPoint = true;
            
            if(!(isdigit(val.s[i]) || gotPoint || val.s[i]=='-' && (val.s[i+1]=='.' || isdigit(val.s[i+1]))))
            {
                if(i && i==len-1 && isalpha(val.s[i])){
                    // special weird case - a num-zero length numeric string terminated by a letter.
                    // This is an integer with a base character: 15x or 0010b. Characters
                    // supported are d, x, b, o for bases 10,16,2,8.
                    char basechar = val.s[i];
                    val.s[i]=0;
                    int x;
                    switch(basechar){
                    case 'd':x=atoi(val.s);break;
                    case 'h':
                    case 'x':x=strtol(val.s,NULL,16);break;
                    case 'b':x=strtol(val.s,NULL,2);break;
                    case 'o':x=strtol(val.s,NULL,8);break;
                    default:
                        curtype=identtoken; // return as ident if bad char!
                        return curtype;
                    }
                    val.i=x;
                    curtype=inttoken;
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
        }
        
        if(gotPoint){
            float f = atof(val.s);
            val.f= f;
            curtype = floattoken;
        } else {
            val.i= atoi(val.s);
            curtype = inttoken;
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
    
    for(k=tokens;k->word;k++)
    {
        if(*(k->word) == '*')
        {
            switch(k->word[1])
            {
            case 'i': identtoken = k->token; break;
            case 's': stringtoken = k->token; break;
            case 'n': inttoken = k->token; break;
            case 'f': floattoken = k->token; break;
            case 'e': endtoken = k->token;break;
            case 'c':
                {
                    dprintf("registering %c",k->word[2]);
                    char c = k->word[2];
                    chartable[c&0x7f]=k->token;
                }
                break;
            default: // ignore badly formed entries
                break;
            }
        }
    }
}




