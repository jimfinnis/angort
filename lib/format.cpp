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

namespace angort {

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

static void formatSignedLong(char *s,long i,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    sprintf(format,"%%%s%dld",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,i);
}

static void formatUnsignedLong(char *s,unsigned long i,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    sprintf(format,"%%%s%dlu",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,i);
}

static void formatHexInt(char *s,unsigned int i,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    sprintf(format,"%%%s%dx",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,i);
}
static void formatHexLong(char *s,unsigned long i,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    sprintf(format,"%%%s%dlx",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,i);
}

static void formatFloat(char *s,double f,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    if(precision!=-9999)
        sprintf(format,"%%%s%d.%df",zeropad?"0":"",negpad?-width:width,precision);
    else
        sprintf(format,"%%%s%df",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,f);
}

static void formatFloat2(char *s,double f,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    if(precision!=-9999)
        sprintf(format,"%%%s%d.%dg",zeropad?"0":"",negpad?-width:width,precision);
    else
        sprintf(format,"%%%s%dg",zeropad?"0":"",negpad?-width:width);
    sprintf(s,format,f);
}

static void formatScientific(char *s,double f,int width,int precision, bool zeropad, bool negpad){
    char format[32];
    if(precision!=-9999)
        sprintf(format,"%%%s%d.%de",zeropad?"0":"",negpad?-width:width,precision);
    else
        sprintf(format,"%%%s%de",zeropad?"0":"",negpad?-width:width);
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
        throw RUNT(EX_TYPE,"format must be a string");
    
    const char *format = Types::tString->getData(formatVal);
    
    ArrayListIterator<Value> iter(items);
    ReadLock lock(items);
    iter.first();
    
    unsigned int width,precision;
    
    // pass 1, work out the buffer size
    int size=0;
    int strsz;
    for(const char *f = format;*f;f++){
        precision=0;
        width=0;
        if(*f=='%'){
            ++f;
            if(*f=='-')f++;
            while(isdigit(*f))
                width = (width*10) + (*f++ - '0');
            if(*f=='.'){
                f++;
                while(isdigit(*f))
                    precision = (precision*10) + (*f++ - '0');
            }
            if((*f=='l' || *f=='z') && (f[1]=='d' || f[1]=='u' || f[1]=='x'))
                ++f; // skip length flag (dealt with in pass 2)
            switch(*f){
            case 'c':
                // fall through
            case '%':
                size++;
                break;
            case 'd':case 'u':case 'x':
                // 50 should be plenty
                iter.next();
                size+=max(50,width);
                break;
            case 'f':
            case 'g':
            case 'e':
                iter.next();
                size+=50; // got to draw the line somewhere.
                break;
            case 's':{
                const StringBuffer& b = iter.current()->toString();
                strsz = max(strlen(b.get()),width);
                size+=strsz;
                iter.next();
                break;
            }
            default://unknown code; throw.
                throw RUNT(EX_BADPARAM,"").set("unknown format in format specification: %c",*f);
            }
        } else
            size++;
    }
    
    // now allocate a temporary buffer
    char *base=(char *)malloc(size+2);
    memset(base,0,size+1);
    char *s = base;
    
    // pass 2 - do the formatting
    iter.first();
    for(const char *f = format;*f;f++){
        if(*f=='%'){
            f++;
            bool isLong=false;
            bool zeropad=false;
            bool negpad=false;
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
            // length flags
            if((*f=='l' || *f=='z') && (f[1]=='d' || f[1]=='u' || f[1]=='x')){
                if(*f=='l')isLong=true;
                ++f;
            }
            
            switch(*f){
            case '%':
                *s++ = '%';
                break;
            case 'c':
                *s++ = iter.current()->toInt();
                iter.next();
                break;
            case 'd':
                if(isLong)
                    formatSignedLong(s,iter.current()->toLong(),width,precision,zeropad,negpad);
                else
                    formatSignedInt(s,iter.current()->toInt(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 'u':
                if(isLong)
                    formatUnsignedLong(s,iter.current()->toLong(),width,precision,zeropad,negpad);
                else
                    formatUnsignedInt(s,iter.current()->toInt(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 'x':
                if(isLong)
                    formatHexInt(s,iter.current()->toInt(),width,precision,zeropad,negpad);
                else
                    formatHexLong(s,iter.current()->toInt(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 'f':
                formatFloat(s,iter.current()->toDouble(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 'g':
                formatFloat2(s,iter.current()->toDouble(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 'e':
                formatScientific(s,iter.current()->toDouble(),width,precision,zeropad,negpad);
                s+=strlen(s);
                iter.next();
                break;
            case 's':{
                const StringBuffer& b = iter.current()->toString();
                formatString(s,b.get(),width,precision,negpad);
                s+=strlen(s);
                iter.next();
                break;
            }
            default:
                throw RUNT(EX_WTF,"unknown spec char in pass 2 of format - should have been caught in pass 1");
            }
        } else
            *s++ = *f;
    }
    *s=0;
    Types::tString->set(out,base);
    free(base);
}

}
