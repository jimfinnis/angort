/**
 * @file plugin.cpp
 * @brief  Example of a plugin library
 *
 */

#include <stdio.h>
#include "../include/plugins.h"


static PluginValue addFunc(PluginValue *params){
    int x = params[0].getInt();
    int y = params[1].getInt();
    
    PluginValue p;
    p.setInt(x+y);
    return p;
}


///////////////////////////////////////////////////////////////////

PluginFunc funcs[] = {
    {"add",addFunc,2},
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
