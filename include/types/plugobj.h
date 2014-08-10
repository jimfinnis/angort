/**
 * @file plugobj.h
 * @brief  GC wrapper object for an object coming out of a plugin
 *
 */

#ifndef __PLUGOBJ_H
#define __PLUGOBJ_H

#include "plugins.h"

namespace angort {

struct PluginObjectWrapper : public GarbageCollected {
    /// to be deleted when I am deleted
    PluginObject *obj;
    
    PluginObjectWrapper(PluginObject *o){
        obj=o;
    }
    
    ~PluginObjectWrapper(){
        if(obj){
            delete obj;
        }
    }
};

class PluginObjectType : public GCType {
public:
    PluginObjectType(){
        add("pluginobject");
    }
    
    PluginObjectWrapper *get(const Value *v);
    void set(Value *v,PluginObjectWrapper *o);
    
};

}
#endif /* __PLUGOBJ_H */
