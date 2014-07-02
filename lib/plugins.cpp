/**
 * @file plugins.cpp
 * @brief  Handles shared library plugins
 *
 */

#include "angort.h"
#include "hash.h"
#include "plugins.h"

#ifdef LINUX
#include <dlfcn.h>



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
    
    // init the plugin and get the data
    PluginInfo *info = (*init)();
    
    // create the namespace
    int ns = names.create(info->name);
    names.push(ns);
    
    // and add the functions. Irritatingly this
    // provides no way of accessing the description/specification.
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
    case PV_SYMBOL:
        Types::tSymbol->set(out,
                            Types::tSymbol->getSymbol(in->getString()));
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
            // we copy the list out into an Angort list.
            ArrayList<Value> *list = Types::tList->set(out);
            PluginValue *q;
            for(PluginValue *p = in->v.head;p;p=q){
                q = p->next;
                Value *dest = list->append();
                pluginToAngort(dest,p);
            }
        }
        break;
    case PV_HASH:
        {
            // we copy the list out into an Angort hash
            Hash *h = Types::tHash->set(out);
            PluginValue *key,*value;
            for(key = in->v.head;key;key=value->next){
                value = key->next;
                Value aVal,aKey;
                pluginToAngort(&aKey,key);
                pluginToAngort(&aVal,value);
                h->set(&aKey,&aVal);
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

/*DOING THIS SO I CAN PASS LISTS TO PLUGINS (HASHES?)
WILL BE RECURSIVE. THEN CAN IMPLEMENT ADD IN MPC,
AND REWORK SEARCH SO IT TAKES A LIST/HASH.
 */
static void angortToPlugin(PluginValue *out,Value *in);

static void angortToPlugin(PluginValue *out,Value *in){
    if(in->t == Types::tInteger)
        out->setInt(in->v.i);
    else if(in->t == Types::tFloat)
        out->setFloat(in->v.f);
    else if(in->t == Types::tNone)
        out->setNone();
    else if(in->t == Types::tString)
        out->setString(Types::tString->getData(in));
    else if(in->t == Types::tSymbol)
        out->setString(Types::tSymbol->getString(in->v.i));
    else if(in->t == Types::tList){
        out->setList();
        ArrayList<Value> *list = Types::tList->get(in);
        for(int i=0;i<list->count();i++){
            Value *v = list->get(i);
            // responsibility to the plugin to delete
            PluginValue *pv = new PluginValue();
            angortToPlugin(pv,v);
            out->addToList(pv);
        }
    } else if(in->t == Types::tHash){
        out->setHash();
        Hash *h = Types::tHash->get(in);
        HashKeyIterator iter(h);
        
        for(iter.first();!iter.isDone();iter.next()){
            Value *key = iter.current();
            h->find(key); // must be there!
            Value *val = h->getval();
            PluginValue *pv = new PluginValue();
            angortToPlugin(pv,key);
            out->addToList(pv);
            pv = new PluginValue();
            angortToPlugin(pv,val);
            out->addToList(pv);
            
        }
    } else if(in->t == Types::tPluginObject){
        out->setObject(in->v.plobj->obj);
    } else
        // inconvertible type, set to None
        out->setNone(); 
}

void Angort::callPlugin(const PluginFunc *native){
    // pop the arguments in reverse order, converting
    PluginValue v[16];
    PluginValue *p=v+native->nargs-1;
    for(int i=0;i<native->nargs;i++,p--){
        Value *a = popval();
        angortToPlugin(p,a);
    }
    
    // call the plugin
    
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
