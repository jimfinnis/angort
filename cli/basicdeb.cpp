/**
 * @file basicdeb.cpp
 * @brief Basic debugger, can be replaced with setDebuggerHook()
 *
 */
#include "angort.h"
#include "debtoks.h"
#include "../lib/opcodes.h"

#include <histedit.h>

using namespace angort;

// we have our own instance of editline
static EditLine *el=NULL;
static History *hist=NULL;
Tokeniser tok;

static const char *getprompt(){return "] ";}

namespace debugger {
static void disasm(Angort *a){
    const Instruction *ip = a->wordbase;
    const Instruction *base = a->wordbase;
    for(;;){
        int opcode = ip->opcode;
        a->showop(ip++,base,a->ip);
        printf("\n");
        if(opcode == OP_END)break;
    }
}

static void process(const char *line,Angort *a){
    tok.reset(line);
    char buf[256];
    for(;;){
        switch(tok.getnext()){
        case T_END:return;
        case T_DUMP:
            a->dumpStack("<debug>");
            break;
        case T_DISASM:
            disasm(a);
            break;
        case T_QUESTION:
            if(tok.getnextident(buf)){
                int idx = a->findOrCreateGlobal(buf);
                Value *v = a->names.getVal(idx);
                v->dump();
            } else {
                printf("expected ident, not %s\n",tok.getstring());
            }
            break;
        case T_FRAME:
            a->dumpFrame();
        default:
            printf("Unknown command\n");
        }
    }
}

}
void basicDebugger(Angort *a){
    tok.init();
    tok.settokens(debtoks);
    tok.setname("<debugger>");
    
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
            debugger::process(line,a);
        }
    }
}

