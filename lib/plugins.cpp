/**
 * @file plugins.cpp
 * @brief  Handles shared library plugins
 *
 */

#include "angort.h"
#include "plugins.h"

#ifdef LINUX
#include <dlfcn.h>

void Angort::plugin(const char *path){
    char *err;
    void *lib = dlopen(path,RTLD_LAZY);
    if(err=dlerror()){
        throw RUNT(err);
    }
    
    PluginInitFunc init = (PluginInitFunc)dlsym(lib,"init");
    if(err=dlerror()){
        throw RUNT(err);
    }
    
    // init the plugin and get the data
    PluginInfo *info = (*init)();
    
    // create the namespace
    int ns = names.create(info->name);
    names.push(ns);
    
    // and add the functions
    PluginFunc *f=info->funcs;
    while(f->name){
        int idx = names.addConst(f->name);
        Value *v = names.getVal(idx);
        Types::tNative->set(v,f);
        f++;
    }
    names.pop();
    pushInt(ns); // leave the NS on the stack
}

void Angort::callPlugin(const PluginFunc *native){
    // pop the arguments in reverse order, converting
    PluginValue v[16];
    PluginValue *p=v;
    for(int i=native->nargs-1;i>=0;i--,p++){
        Value *a = popval();
        if(a->t == Types::tInteger)
            p->setInt(a->v.i);
        else if(a->t == Types::tFloat)
            p->setFloat(a->v.f);
        else if(a->t == Types::tString)
            p->setString(a->v.s); // no copy
        else if(a->t == Types::tPluginObject){
            p->setObject(a->v.plobj->obj);
        } else
            throw RUNT("").set("not a permitted type in plugins: %s",a->t->name);
    }
    
    // call
    PluginValue result;
    (*native->func)(&result,v);
    
    // and process the return
    
    switch(result.type){
    case PV_INT:
        pushInt(result.getInt());
        break;
    case PV_FLOAT:
        pushFloat(result.getFloat());
        break;
    case PV_STRING:
        // makes a copy on the stack
        pushString(result.getString());
        break;
    case PV_OBJ:
        {
            PluginObject *obj = result.getObject();
            if(!(obj->wrapper)){
                // no wrapper exists, create a new one
                obj->wrapper = new PluginObjectWrapper(obj);
            }
            Value *v = pushval();
            Types::tPluginObject->set(v,obj->wrapper);
            
        }
    case PV_NONE:break;
        break;
    default:
        throw RUNT("").set("return from plugin invalid pv-type: %d",result.type);
    }
}

#else

void Angort::callPlugin(const PluginFunc *p){
    throw WTF;
}

void Angort::plugin(const char *path){
    throw RUNT("Plugins not supported on this platform.");
}
#endif
