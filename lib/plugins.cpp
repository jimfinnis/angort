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


static void pluginToAngort(Value *out, PluginValue *in){
    switch(in->type){
    case PV_INT:
        Types::tInteger->set(out,in->getInt());
        break;
    case PV_FLOAT:
        Types::tFloat->set(out,in->getFloat());
        break;
    case PV_STRING:
        Types::tString->set(out,in->getString());
        break;
    case PV_OBJ:
        {
            PluginObject *obj = in->getObject();
            if(!(obj->wrapper)){
                // no wrapper exists, create a new one
                obj->wrapper = new PluginObjectWrapper(obj);
            }
            Types::tPluginObject->set(out,obj->wrapper);
            
        }
        break;
    case PV_LIST:
        {
            // we copy the list out into an Angort list,
            // and delete the nodes. We delete any strings
            // indexed by the nodes.
            ArrayList<Value> *list = Types::tList->set(out);
            PluginValue *q;
            for(PluginValue *p = in->v.head;p;p=q){
                q = p->next;
                Value *dest = list->append();
                pluginToAngort(dest,p);
                if(p->type == PV_STRING)
                    free((void *)p->v.s);
                delete p;
            }
        }
        break;
    case PV_NONE:
        out->clr();
        break;
    default:
        throw RUNT("").set("return from plugin invalid pv-type: %d",in->type);
    }
    
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
            p->setString(Types::tString->getData(a)); // no copy
        else if(a->t == Types::tPluginObject){
            p->setObject(a->v.plobj->obj);
        } else
            throw RUNT("").set("not a permitted type in plugins: %s",a->t->name);
    }
    
    // call
    
    try {
        PluginValue result;
        (*native->func)(&result,v);
        // and process the return
        if(result.type != PV_NORETURN){
            Value *a = pushval();
            pluginToAngort(a,&result);
        }
    } catch(const char *s){
        throw RUNT(s);
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
