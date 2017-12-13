/**
 * @file debugger.cpp
 * @brief Basic debugger, can be replaced with setDebuggerHook()
 *
 */
#include "angort.h"
#include "debtoks.h"
#include "../lib/opcodes.h"

#include <histedit.h>
#include <signal.h>

#include "completer.h"

using namespace angort;

bool debuggerBreakHack=false;

// we have our own instance of editline
static EditLine *el=NULL;
static History *hist=NULL;
Tokeniser tok;

static const char *getprompt(){return "] ";}

static const char *usage = 
"quit          exit debugger and continue program\n"
"abort         terminate program (remain in debugger)\n"
"terminate     terminate angort completely\n"
"stack         show the stack\n"
"<n> print     detailed view of stack entry <n>\n"
"?Var          detailed view of global <Var>\n"
"disasm        disassemble current function\n"
"b             toggle breakpoint on current IP\n"
"n             next instruction\n"
"<string> f    break on named function\n"
"bt            backtrace\n"
"frame         show context frame\n"
"CTRL-D        exit debugger and continue program (if not aborted)\n"
"h             show this string\n"
;


namespace debugger {

static bool exitDebug=false;

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
    Stack<Value,8> stack;
    a->debuggerStepping=false;
    for(;;){
        try{
            switch(tok.getnext()){
            case T_BREAK:
                ((Instruction *)a->ip)->brk= !a->ip->brk;
                break;
            case T_INT:
                Types::tInteger->set(stack.pushptr(),tok.getint());
                break;
            case T_STRING:
                Types::tString->set(stack.pushptr(),tok.getstring());
                break;
            case T_END:return;
            case T_HELP:puts(usage);break;
            case T_STACK:
                a->dumpStack("<debug>");
                break;
            case T_BACKTRACE:
                a->printStoredTrace();
                break;
            case T_QUIT:
                exitDebug=true;
                a->debuggerStepping=false;
                a->debuggerNextIP=false;
                break;
            case T_TERMINATE:
                delete a;
                exit(0);
            case T_NEXT:
                exitDebug=true;
                a->debuggerStepping=true;
                a->debuggerNextIP=true;
                break;
            case T_FUNC:
                {
                    const StringBuffer& b = stack.popptr()->toString();
                    // find the global
                    Value *v = a->findOrCreateGlobalVal(b.get());
                    Instruction *ip;
                    if(v->t == Types::tCode){
                        ip = (Instruction*)v->v.cb->ip;
                    } else if(v->t == Types::tClosure){
                        ip = (Instruction*)v->v.closure->cb->ip;
                    } else printf("expected the name of a function\n");
                    ip->brk = !ip->brk;
                }
                break;
            case T_PRINT:
                i=stack.popptr()->toInt();
                if(i<0){
                    printf("expected +ve integer stack index\n");
                } else {
                    char *p=NULL;
                    Value *v = a->stack.peekptr(i);
                    printf("Type: %s\n",v->t->name);
                    v->dump(&p);
                    printf("%s\n",p);
                    free(p);
                }
                break;
            case T_DISASM:
                disasm(a);
                break;
            case T_QUESTION:
                if(tok.getnextident(buf)){
                    char *p=NULL;
                    int idx = a->findOrCreateGlobal(buf);
                    Value *v = a->names.getVal(idx);
                    v->dump(&p);
                    printf("%s\n",p);
                    free(p);
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
class DebuggerAutocomplete : public completer::Iterator {
    int idx,l;
    const char *strstart;
public:
    virtual void first(const char *stringstart,int len){
        idx=0;
        strstart = stringstart;
        l = len;
    }
    virtual const char *next(){
        while(debtoks[idx].word && (debtoks[idx].word[0]=='*' ||
              strncmp(strstart,debtoks[idx].word,l)))
            idx++;
        if(!debtoks[idx].word)return NULL;
        return debtoks[idx++].word;
    }
};


}

static void debugSighandler(int s)
{
    extern void cliShutdown();
    printf("Signal caught in debugger\n");
    debugger::exitDebug=true;
//    el_end(el);
//    cliShutdown();
//    exit(1);
}

void basicDebugger(Angort *a){
    struct sigaction sa,oldsa;
    sa.sa_handler = debugSighandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV,&sa,&oldsa);
    sigaction(SIGINT,&sa,NULL);
    tok.init();
    tok.settokens(debtoks);
    tok.setname("<debugger>");
    
    if(!a->debuggerStepping){
        printf("Debugger: TAB-TAB for command list, ctrl-D to exit and continue.\n"
               "abort to terminate program (remaining in debugger), ?var to query\n"
               "global variable.\n\n");
    }
    
    if(a->ip){
        a->showop(a->ip);putchar('\n');
        a->printTrace();
    }else{
        printf("No IP - exception?\n");
        a->printStoredTrace();
    }
    
    
    HistEvent ev;
    // make our editline
    el = el_init("angort-debugger",stdin,stdout,stderr);
    el_set(el,EL_PROMPT,&getprompt);
    el_set(el,EL_EDITOR,"emacs");
    if(!hist){
        hist = history_init();
        history(hist,&ev,H_SETSIZE,800);
        el_set(el,EL_HIST,history,hist);
        if(!hist)
            printf("warning: no history\n");
        
        
    }
    static debugger::DebuggerAutocomplete compr;
    completer::setup(el,&compr,"\t\n ");
    
    debugger::exitDebug=false;
    while(!debugger::exitDebug){
        int count;
        debuggerBreakHack=true;
        const char *line = el_gets(el,&count);
        if(!line)break;
        debuggerBreakHack=false;
        if(hist){
            if(count>1){ // trailing newline
                history(hist,&ev,H_ENTER,line);
                debugger::process(line,a);
            } else {
                if(history(hist,&ev,H_LAST)>=0)
                    debugger::process(ev.str,a);
            }
        } else {
            debugger::process(line,a);
        }
    }
    putchar('\r');
    
    sigaction(SIGSEGV,&oldsa,NULL);
    sigaction(SIGINT,&oldsa,NULL);
    el_end(el);
}

