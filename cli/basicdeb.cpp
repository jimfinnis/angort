/**
 * @file basicdeb.cpp
 * @brief Basic debugger, can be replaced with setDebuggerHook()
 *
 */
#include "angort.h"

#include <histedit.h>

using namespace angort;

// we have our own instance of editline
static EditLine *el=NULL;
static History *hist=NULL;

static const char *getprompt(){return "] ";}

static void process(const *char line,Angort *a){
}


void basicDebugger(Angort *a){
    HistEvent ev;
    if(!el){
        // make our editline if we don't have one
        el = el_init("angort-debugger",stdin,stdout,stderr);
        el_set(el,EL_PROMPT,&getprompt);
        el_set(el,EL_EDITOR,"emacs");
    
        hist = history_init();
        history(hist,&ev,H_SETSIZE,800);
        el_set(el,EL_HIST,history,hist);
    
        if(!hist)
            printf("warning: no history\n");
    }
    
    for(;;){
        int count;
        const char *line = el_gets(el,&count);
        if(!line)break;
        if(count>1){ // trailing newline
            if(hist)
                history(hist,&ev,H_ENTER,line);
            process(line,a);
        }
    }
}

