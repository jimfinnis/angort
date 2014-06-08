/**
 * @file
 * String formatting code.
 * Much of this code was adapted from Python's string formatting
 * code, but the actual formatting of individual items is done
 * by constructing printf formats and using them. Ugly, but easy.
 * There may be some dodgy assumptions about lengths here.
 */

#include "angort.h"
#include <ctype.h>
#define max(a,b) ((a)>(b)?(a):(b))

#define ITEMLEN 1024
static char buf[ITEMLEN];

static void formatSignedInt(char *s,int i,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    sprintf(format,"%%%s%dd",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,i);
}

static void formatUnsignedInt(char *s,unsigned int i,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    sprintf(format,"%%%s%du",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,i);
}

static void formatHexInt(char *s,unsigned int i,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    sprintf(format,"%%%s%dx",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,i);
}

static void formatFloat(char *s,float f,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    if(precision!=-9999)
        sprintf(format,"%%%s%d.%df",zeropad?"0":"",negpad?-width:width,precision);
    else
        sprintf(format,"%%%s%df",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,f);
}

static void formatString(char *s,const char *in,int width,int precision, bool negpad){
    char format[32];
    snprintf(format,20,"%%%ds",negpad?-width:width);
    sprintf(s,format,in);
}



void format(Value *out,Value *formatVal,ArrayList<Value> *items){
    
    // sorry, it actually has to be a string.
    if(formatVal->t != Types::tString)
        throw RUNT("format must be a string");
    
    const char *format = Types::tString->getData(formatVal);
    const char *t;
    
    ArrayListIterator<Value> iter(items);
    iter.first();
    
    int width,precision;
    
    // pass 1, work out the buffer size
    int size=0;
    int strsz;
    for(const char *f = format;*f;f++){
        precision=0;
        width=0;
        if(*f=='%'){
            const char *p = f++;
            if(*f=='-')f++;
            while(isdigit(*f))
                width = (width*10) + (*f++ - '0');
            if(*f=='.'){
                f++;
                while(isdigit(*f))
                    precision = (precision*10) + (*f++ - '0');
            }
            while(*f && *f!='%' && !isalpha(*f))
                ;
            if((*f=='l' || *f=='z') && (f[1]=='d' || f[1]=='u'))
                ++f;
            switch(*f){
            case 'c':
                iter.next();
                // fall through
            case '%':
                size++;
                break;
            case 'd':case 'u':case 'x':
                // 20 is enough for a 64bit int
                iter.next();
                size+=max(20,width);
                break;
            case 'f':
                iter.next();
                size+=20; // got to draw the line somewhere.
                break;
            case 's':
                strsz = max(strlen(iter.current()->toString(buf,ITEMLEN)),width);
                size+=strsz;
                iter.next();
                break;
            default://unknown code; just add the rest of the string in. Ugh.
                size+=strlen(p);
                goto expand;
                break;
            }
        } else
            size++;
    }
expand:
    // now allocate a temporary buffer
    char *base=(char *)malloc(size+2);
    memset(base,0,size+1);
    char *s = base;
    
    // pass 2 - do the formatting
    iter.first();
    for(const char *f = format;*f;f++){
        if(*f=='%'){
            bool zeropad=false;
            bool negpad=false;
            const char *p = f++;
            int i;
            precision=-9999; // rogue value for "no precision"
            width=0;
            if(*f=='-'){
                negpad=true;
                f++;
            }
            if(*f=='0')
                zeropad=true;
            while(isdigit(*f))
                width = (width*10) + (*f++ - '0');
            if(*f=='.'){
                precision=0;
                f++;
                while(isdigit(*f))
                    precision = (precision*10) + (*f++ - '0');
            }
            while(*f && *f!='%' && !isalpha(*f))
                f++;
            // skip length flags
            if((*f=='l' || *f=='z') && (f[1]=='d' || f[1]=='u'))
                ++f;
            
            switch(*f){
            case 'c':
                *s++ = iter.current()->toInt();
                iter.next();
                break;
            case 'd':
                formatSignedInt(s,iter.current()->toInt(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 'u':
                formatUnsignedInt(s,iter.current()->toInt(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 'x':
                formatHexInt(s,iter.current()->toInt(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 'f':
                formatFloat(s,iter.current()->toFloat(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 's':
                t = iter.current()->toString(buf,ITEMLEN);
                formatString(s,t,width,precision,negpad);
                s+=strlen(s);
                iter.next();
                break;
            default:
                strcpy(s,p);
                s+=strlen(s);
                goto end;
            }
        } else
            *s++ = *f;
    }
end:
    *s=0;
    Types::tString->set(out,base);
    free(base);
}
