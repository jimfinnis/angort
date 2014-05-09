/**
 * @file plugins.cpp
 * @brief  Handles shared library plugins
 *
 */

#include "angort.h"
#include "plugins.h"

#ifdef LINUX
#include <dlfcn.h>
#endif

void Angort::plugin(const char *path){
#ifndef LINUX
    throw RUNT("Plugins not supported on this platform.");
#else
    char *err;
    void *lib = dlopen(path,RTLD_LAZY);
    if(err=dlerror()){
        throw RUNT(err);
    }
    
    PluginInitFunc init = (PluginInitFunc)dlsym(lib,"init");
    if(err=dlerror()){
        throw RUNT(err);
    }
    
    // get the plugin word data
    PluginEntry *ents = (*init)();
    
    
#endif
}
