/**
 * @file plugins.cpp
 * @brief  Handles shared library plugins
 *
 */

#include "angort.h"
#include "hash.h"

#ifdef LINUX
#include <dlfcn.h>

namespace angort {

typedef LibraryDef *(*PluginInitFunc)(class Angort *a);


void Angort::plugin(const char *name){
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
    int nsidx = registerLibrary(info);
}

#else

void Angort::plugin(const char *path){
    throw RUNT("Plugins not supported on this platform.");
}

#endif

}
