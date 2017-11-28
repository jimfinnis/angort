/**
 * @file basicdeb.cpp
 * @brief Basic debugger, can be replaced with setDebuggerHook()
 *
 */
#include "angort.h"
#include "debtoks.h"
#include "../lib/opcodes.h"

#include <histedit.h>

#include "completer.h"

using namespace angort;

bool debuggerBreakHack=false;

// we have our own instance of editline
static EditLine *el=NULL;
static History *hist=NULL;
Tokeniser tok;

static const char *getprompt(){return "] ";}

static const char *usage = 
"abort         terminate program (remain in debugger)\n"
"stack         show the stack\n"
"print <n>     detailed view of stack entry <n>\n"
"?Var          detailed view of global <Var>\n"
"disasm        disassemble current function\n"
"frame         show context frame\n"
"help          show this string\n"
;


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
    int i;
    for(;;){
        try{
            switch(tok.getnext()){
            case T_END:return;
            case T_HELP:puts(usage);break;
            case T_STACK:
                a->dumpStack("<debug>");
                break;
            case T_PRINT:
                i=tok.getnextint();
                if(i<0){
                    printf("expected +ve integer stack index\n");
                } else {
                    Value *v = a->stack.peekptr(i);
                    v->dump();
                }
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
            case T_ABORT:
                a->stop();
                break;
            case T_FRAME:
                a->dumpFrame();
                break;
            default:
                printf("Unknown command %s\n",tok.getstring());
            }
        } catch(Exception e){
            printf("Error: %s\n",e.what());
        }
    }
}

// autocompletion data generator
class DebuggerAutocomplete : public AutocompleteIterator {
    int idx;
public:
    virtual void first(){
        idx=0;
    }
    virtual const char *next(){
        while(debtoks[idx].word && debtoks[idx].word[0]=='*')
            idx++;
        if(!debtoks[idx].word)return NULL;
        return debtoks[idx++].word;
    }
};


}
void basicDebugger(Angort *a){
    tok.init();
    tok.settokens(debtoks);
    tok.setname("<debugger>");
    
    printf("Debugger: TAB-TAB for command list, ctrl-D to exit and continue.\n"
           "abort to terminate program (remaining in debugger, ?var to query\n"
           "global variable.\n\n");
    
    
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
        
        static debugger::DebuggerAutocomplete completer;
        setupAutocomplete(el,&completer,"\t\n ");
        
    }
    
    for(;;){
        int count;
        debuggerBreakHack=true;
        const char *line = el_gets(el,&count);
        if(!line)break;
        debuggerBreakHack=false;
        if(count>1){ // trailing newline
            if(hist)
                history(hist,&ev,H_ENTER,line);
            debugger::process(line,a);
        }
    }
    putchar('\r');
}

