/**
 * @file plugins.cpp
 * @brief  Handles shared library plugins
 *
 */

#include "angort.h"
#include "hash.h"

#include <unistd.h>

namespace angort {
void Runtime::popParams(Value **out,const char *spec,const Type *type0,
                       const Type *type1) {
    
    const char *p = spec;
    Value *v;
    const Type *tt;
    
    int i=strlen(p)-1;
    p = spec+i;
    while(p>=spec){
        v = popval();
        
//        printf("Argument %d: %s\n",i,v->toString().get());
        
        switch(*p){
        case 'n':
        case 'd':
        case 'i':
        case 'L':
            if(!(v->t->flags & TF_NUMBER))
                throw ParameterTypeException(i,"number",v->t->name);
            break;
        case 'N':
            if(!(v->t->flags & TF_NUMBER) && v->t!=Types::tNone)
                throw ParameterTypeException(i,"number or none",v->t->name);
            break;
        case 'y':
            if(v->t != Types::tString && v->t != Types::tSymbol && !v->isNone())
                throw ParameterTypeException(i,"string",v->t->name);
            break;
        case 's':
            if(v->t != Types::tString && v->t != Types::tSymbol )
                throw ParameterTypeException(i,"string",v->t->name);
            break;
        case 'S':
            if(v->t != Types::tSymbol )
                throw ParameterTypeException(i,"symbol",v->t->name);
            break;
        case 'c':
            if(v->t != Types::tCode && v->t != Types::tClosure )
                throw ParameterTypeException(i,"function",v->t->name);
            break;
        case 'C':
            if(v->t != Types::tCode && v->t != Types::tClosure && v->t != Types::tNone)
                throw ParameterTypeException(i,"function or none",v->t->name);
            break;
        case 'l':
            if(v->t != Types::tList )
                throw ParameterTypeException(i,"list",v->t->name);
            break;
        case 'h':
            if(v->t != Types::tHash )
                throw ParameterTypeException(i,"hash",v->t->name);
            break;
        case 'I':
            if(!(v->t->flags & TF_ITERABLE))
                throw ParameterTypeException(i,"iterable",v->t->name);
            break;
        case 'a':
        case 'b':
        case 'A':
        case 'B':
            // special types - a or A is type0, b or B is type1. If the character is lower
            // case we permit NONE.
            // which of the two possible special types?
            tt = (*p=='a' || *p=='A')?type0:type1;
            if(!tt)
                throw ParameterTypeException(i,"unsupplied special type specified in parameter check",v->t->name);
            // if the parameter is upper case, we don't allow NONE through.
            if(*p=='A' || *p=='B'){
                if(v->t != tt)
                    throw ParameterTypeException(i,tt->name,v->t->name);
            } else {
                if((v->t != tt) && v->t != Types::tNone)
                    throw ParameterTypeException(i,tt->name,v->t->name,true);
            }
            break;
                
        case 'v':
        case '?':
            break;
        default:
            throw ParameterTypeException(i,"an impossible parameter",v->t->name);
        }
        out[i--]=v;
        p--;
    }
}


}
#ifdef LINUX
#include <dlfcn.h>
#include <limits.h>

namespace angort {

typedef LibraryDef *(*PluginInitFunc)(class Angort *a);


int Angort::plugin(const char *name){
    char *err;
    const char *path;
    char buf[256];
    
    snprintf(buf,256,"%s.angso",name);
    path = findFile(buf);
    
    if(!path)
        throw RUNT(EX_NOTFOUND,"").set("cannot find library '%s'",name);
    
    // ugly hackage; dlopen() doesn't seem to like plain filenames
    // (e.g. "io.angso")
    char apath[PATH_MAX];
    char *pp = realpath(path,apath);
    if(!pp)
        throw RUNT(EX_LIBRARY,"").set("couldn't resolve path: %s",path);
    
    
    void *lib = dlopen(apath,RTLD_LAZY|RTLD_GLOBAL);
    if((err=dlerror())){
        throw RUNT(EX_LIBRARY,err);
    }
    
    PluginInitFunc init = (PluginInitFunc)dlsym(lib,"init");
    if((err=dlerror())){
        throw RUNT(EX_LIBRARY,err);
    }
    
    // init the plugin and get the data, and register it.
    LibraryDef *info = (*init)(this);
    
    return registerLibrary(info);
}

#else

void Angort::plugin(const char *path){
    throw RUNT(EX_LIBRARY,"Plugins not supported on this platform.");
}

#endif

}
