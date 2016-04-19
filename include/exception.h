/**
 * \file 
 *
 * 
 * \author $Author$
 * \date $Date$
 */


#ifndef __ANGORTEXCEPTION_H
#define __ANGORTEXCEPTION_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <exception>

#include "exceptsymbs.h"

namespace angort {

extern int getSymbolID(const char *);

class Exception : public std::exception {
public:
    Exception(const char *xid,const char *e){
        id = getSymbolID(xid);
        if(e)
            strncpy(error,e,1024);
        else
            strcpy(error,"???");
        fatal=false;
    }
    
    /// a variadic fluent modifier to set a better string with sprintf
    Exception& set(const char *s,...){
        va_list args;
        va_start(args,s);
        
        vsnprintf(error,1024,s,args);
        va_end(args);
        return *this;
    }
    
    /// default ctor
    Exception(const char *xid){
        fatal=false;
        id = getSymbolID(xid);
        strcpy(error,"???");
    }
    
    /// construct the exception. Note that
    /// space is allocated here, which is deleted
    /// by the destructor. This version considers e to be
    /// a format string, and arg a string argument.
    Exception(const char *xid,const char *e,const char *arg){
        fatal=false;
        id = getSymbolID(xid);
        snprintf(error,1024,e,arg);
    }
    
    /// return the error string
    virtual const char *what () const throw (){
        return error;
    }
    
    /// a copy of the error string
    char error[1024];
    bool fatal;
    int id; // ID of symbol
};

#define RUNT(xid,x) RuntimeException(xid,x,__FILE__,__LINE__)
#define WTF RuntimeException(EX_WTF,"What a Terrible Failure",__FILE__,__LINE__)

/// this exception is thrown when a generic runtime error occurs.
/// It can be caught by Angort exception handling.

class RuntimeException : public Exception {
public:
    RuntimeException(const char *xid,const char *e,const char *fn,int l) :
    Exception(xid)
    {
        char tmp[512];
        if(e){
            strcpy(tmp,e);e=tmp;  // to stop bloody exp-ptrcheck complaining.
        }
    
        if(fn)
            strncpy(fileName,fn,1024);
        else
            strcpy(fileName,"???");
        line = l;
        
        snprintf(error,1024,"%s(%d):  %s",fileName,line,
                 e?e:"unknown");
        fatal=false;
    }
   
    char fileName[1024];
    /// the current Angort line or -1
    int line;
};

/*
 * 
 * 
 * Standard exception types
 * 
 * 
 *
 */



class BadConversionException : public Exception {
public:
    BadConversionException(const char *from, const char *to)
                : Exception(EX_BADCONV) {
                    sprintf(error,"cannot convert types: '%s' to '%s'",from,to);
                }
};

class DivZeroException : public Exception {
public:
    DivZeroException() : Exception(EX_DIVZERO,"division by zero") {}
};

class SyntaxException : public Exception {
public:
    SyntaxException(const char *s) : Exception(EX_SYNTAX,s) {}
};

class AlreadyDefinedException : public Exception {
public:
    AlreadyDefinedException(const char *name) : Exception(EX_DEFINED,NULL){
        set("'%s' is already defined in this namespace");
    }
};

class FileNameExpectedException : public SyntaxException {
public:
    FileNameExpectedException() : 
        SyntaxException("filename expected") {}
};

class FileNotFoundException : public Exception {
public:
    char fname[1024];
    
    FileNotFoundException(const char *name) :  Exception(EX_NOTFOUND) {
        snprintf(error,1023,"cannot find file : %s",name);
        strncpy(fname,name,1023);
        
    }
    
};

class AssertException : public Exception {
public:
    AssertException(const char *desc,int line) : Exception(EX_ASSERT){
        snprintf(error,1024,"Assertion failed at line %d:  %s",line,desc);
        fatal=true;
    }
};

class ParameterTypeException : public Exception {
public:
    ParameterTypeException(int paramNo, const char *expected) : Exception(EX_BADPARAM){
        snprintf(error,1024,"Bad parameter %d, expected %s",paramNo,expected);
    }
};

}

#endif /* __EXCEPTION_H */
