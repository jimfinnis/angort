/**
 * @file plugin.cpp
 * @brief  Example of a plugin library
 *
 */

#include <stdio.h>
#include "../include/plugins.h"

class TestObject : public PluginObject {
public:
    int q;
    void show(){
        printf("And the value is: %d\n",q);
    }
    
    /// needs to be virtual because we destruct through
    /// the PluginObject.
    virtual ~TestObject(){
        printf("deleting test object\n");
    }
};




static void addFunc(PluginValue *res,PluginValue *params){
    int x = params[0].getInt();
    int y = params[1].getInt();
    
    res->setInt(x+y);
}

static void mkobjFunc(PluginValue *res,PluginValue *params){
    TestObject *t = new TestObject();
    res->setObject(t);
}

static void setvalFunc(PluginValue *res,PluginValue *params){
    TestObject *t = (TestObject *)params[0].getObject();
    t->q = params[1].getInt();
}
static void showFunc(PluginValue *res,PluginValue *params){
    TestObject *t = (TestObject *)params[0].getObject();
    t->show();
}


///////////////////////////////////////////////////////////////////

PluginFunc funcs[] = {
    {"add",addFunc,2},
    {"mkobj",mkobjFunc,0},
    {"setval",setvalFunc,2},
    {"show",showFunc,1},
    
    {NULL,NULL,-1} // terminator
};

PluginInfo info = {
    "exampleplugin",funcs
};

/// init function returns the plugin info structure
extern "C" PluginInfo *init(){
    printf("Initialising example plugin\n");
    return &info;
}
