/**
 * @file shell.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/plugins.h"

static void mpcFunc(PluginValue *res,PluginValue *params){
    FILE *fp;
    
    char buf[1024];
    snprintf(buf,1024,"/usr/bin/mpc %s",params[0].getString());
    fp = popen(buf,"r");
    res->setList();
    if(fp!=NULL){
        while(fgets(buf,sizeof(buf)-1,fp)!=NULL){
            PluginValue *pv = new PluginValue();
            buf[strlen(buf)-1]=0; // strip trailing LF
            pv->setString(strdup(buf));
            res->addToList(pv);
        }
        pclose(fp);   
    } else 
        res->setNone();
}

static PluginFunc funcs[]= {
    {"mpc",mpcFunc,1},
    {NULL,NULL,-1}
};

static PluginInfo info = {
    "mpc",funcs
};

extern "C" PluginInfo *init(){
    printf("Initialising MPC plugin\n");
    return &info;
}
   
