/**
 * @file plugins.cpp
 * @brief  Handles shared library plugins
 *
 */

#include "angort.h"
#include "hash.h"


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
            if(v->t != Types::tInteger && v->t != Types::tFloat)
                throw ParameterTypeException(i,"number");
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
        case 'a':
        case 'b':
        case 'A':
        case 'B':
            tt = (*p=='a' || *p=='A')?type0:type1;
            if(!tt)
                throw RUNT("unsupplied special type specified in parameter check");
            // if the parameter is T, we don't allow NONE through.
            if(v->t != tt && (!v->isNone() || *p=='a' || *p=='a'))
                throw ParameterTypeException(i,tt->name);
            break;
                
        case 'v':
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

namespace angort {

typedef LibraryDef *(*PluginInitFunc)(class Angort *a);


int Angort::plugin(const char *name){
    char *err;
    const char *path;
    char buf[256];
    
    snprintf(buf,256,"%s.angso",name);
    path = findFile(buf);
    
    if(!path)
        throw RUNT("").set("cannot find library '%s'",name);
    
    void *lib = dlopen(path,RTLD_LAZY);
    if((err=dlerror())){
        throw RUNT(err);
    }
    
    PluginInitFunc init = (PluginInitFunc)dlsym(lib,"init");
    if((err=dlerror())){
        throw RUNT(err);
    }
    
    // init the plugin and get the data, and register it.
    LibraryDef *info = (*init)(this);
    
    return registerLibrary(info);
}

#else

void Angort::plugin(const char *path){
    throw RUNT("Plugins not supported on this platform.");
}

#endif

}
