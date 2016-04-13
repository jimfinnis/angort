/**
 * @file plugins.cpp
 * @brief  Handles shared library plugins
 *
 */

#include "angort.h"
#include "hash.h"

#include <unistd.h>

namespace angort {
void Angort::popParams(Value **out,const char *spec,const Type *type0,
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
        case 'i':
            if(v->t != Types::tInteger && v->t != Types::tFloat)
                throw ParameterTypeException(i,"number");
            break;
        case 'N':
            if(v->t != Types::tInteger && v->t != Types::tFloat && v->t != Types::tNone)
                throw ParameterTypeException(i,"number or none");
            break;
        case 'y':
            if(v->t != Types::tString && v->t != Types::tSymbol && !v->isNone())
                throw ParameterTypeException(i,"string");
            break;
        case 's':
            if(v->t != Types::tString && v->t != Types::tSymbol )
                throw ParameterTypeException(i,"string");
            break;
        case 'S':
            if(v->t != Types::tSymbol )
                throw ParameterTypeException(i,"symbol");
            break;
        case 'c':
            if(v->t != Types::tCode && v->t != Types::tClosure )
                throw ParameterTypeException(i,"function");
            break;
        case 'l':
            if(v->t != Types::tList )
                throw ParameterTypeException(i,"list");
            break;
        case 'h':
            if(v->t != Types::tHash )
                throw ParameterTypeException(i,"hash");
            break;
        case 'I':
            if(!(v->t->flags & TF_ITERABLE))
                throw ParameterTypeException(i,"iterable");
            break;
        case 'a':
        case 'b':
        case 'A':
        case 'B':
            tt = (*p=='a' || *p=='A')?type0:type1;
            if(!tt)
                throw RUNT("ex$plugin","unsupplied special type specified in parameter check");
            // if the parameter is T, we don't allow NONE through.
            if(v->t != tt && (!v->isNone() || *p=='a' || *p=='a'))
                throw ParameterTypeException(i,tt->name);
            break;
                
        case 'v':
        case '?':
            break;
        default:
            throw ParameterTypeException(i,"an impossible parameter");
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
        throw RUNT("ex$plugin","").set("cannot find library '%s'",name);
    
    // ugly hackage; dlopen() doesn't seem to like plain filenames
    // (e.g. "io.angso")
    char apath[PATH_MAX];
    char *pp = realpath(path,apath);
    if(!pp)
        throw RUNT("ex$plugin","").set("couldn't resolve path: %s",path);
    
    
    void *lib = dlopen(apath,RTLD_LAZY|RTLD_GLOBAL);
    if((err=dlerror())){
        throw RUNT("ex$plugin",err);
    }
    
    PluginInitFunc init = (PluginInitFunc)dlsym(lib,"init");
    if((err=dlerror())){
        throw RUNT("ex$plugin",err);
    }
    
    // init the plugin and get the data, and register it.
    LibraryDef *info = (*init)(this);
    
    return registerLibrary(info);
}

#else

void Angort::plugin(const char *path){
    throw RUNT("ex$plugin","Plugins not supported on this platform.");
}

#endif

}
