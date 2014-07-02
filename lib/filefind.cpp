/**
 * @file filefind.cpp
 * @brief  Looks for files in the search path. In LINUX, will attempt
 * to use wordexp shell expansions; on Windows it might do odd things
 * particularly with absolute paths. Will return a string buffer
 * which must be freed, if successful. Null otherwise.
 */

#include "angort.h"
#ifdef LINUX
#include <wordexp.h>
#include <unistd.h>
#endif

const char *Angort::findFile(const char *name){
    char path[2048];
    const char *p = searchPath?searchPath:DEFAULTSEARCHPATH;
    const char *q;
    
    // don't do any of this if the name is absolute. Will require
    // some work on Windows!
    if(*name=='/'){
        if(!access(path,R_OK))
            return strdup(name);
        else
            return NULL;
        
    }
    
    for(;*p;p=q+1){
        q=p;
        while(*q && *q!=':')q++;
        if(q-p<1024){
            memcpy(path,p,q-p);
            path[q-p]=0;
            
#ifdef LINUX
            // now get a shell expansion
            
            wordexp_t exp;
            if(!wordexp(path,&exp,0)){
                // check all the possible results
                for(unsigned int i=0;i<exp.we_wordc;i++){
                    strncpy(path,exp.we_wordv[i],2047);
                    strncat(path,"/",2047);
                    strncat(path,name,2047);
                    if(!access(path,R_OK)){
                        wordfree(&exp);
                        return strdup(path);
                    }
                }
                wordfree(&exp);
            }
#else
            strncat(path,"/",2048);
            strncat(path,name,2048);
            if(!access(path,R_OK)){
                return strdup(path);
            }
#endif
        }
    }
    return NULL;
}

