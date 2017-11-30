/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <histedit.h>

#include "angort.h"
#include "completer.h"

// keep this up to date!
static const char *copyString="(c) Jim Finnis 2012-2017";
static const char *usageString=
"\nUsage: angort [-h] [-n] [-e] [-d] [-D] [-b] [-if] [-in]\n"
"        [-llib] [-llib]..\n\n"
"-h    : this string\n"
"-n    : execute command-line script in loop, requires two args: init\n"
"        and loop (the latter reads lines from stdin)\n"
"-e    : execute command-line script\n"
"-d    : disassemble while running\n"
"-D    : tokeniser trace\n"
"-b    : signals cause debugger entry rather than exit\n"
"-if   : import symbols from future namespace, mutually exclusive with...\n"
"-if   : import symbols from deprecated namespace\n"
"-llib : import named library\n"
;

using namespace angort;

extern angort::LibraryDef LIBNAME(cli);

// stripped arglist, wrapped by a value. These are linked
// at the start of main().

Value strippedArgVal;
static ArrayList<Value> *strippedArgs;

Angort *a;

static void showException(Exception& e){
    printf("Error: %s\n",e.what());
    const Instruction *ip = a->getIPException();
    if(ip){
        printf("Error at:");
        a->showop(ip);
        printf("\n");
    }else
          printf("Last line input: %s\n",a->getLastLine());
    if(e.fatal)
        exit(1);
    
    a->clearStack();
}

#define F_CMD 1
#define F_LOOP 2
#define F_FUTURE 4
#define F_DEPRECATED 8

// autocompletion data generator
class AngortAutocomplete : public completer::Iterator {
    const char *strstart;
    int l;
public:
    virtual void first(const char *stringstart, int len){
        strstart = stringstart;
        l = len;
        a->resetAutoComplete();
    }
    virtual const char *next(){
        const char *s=NULL;
        do
            s = a->getNextAutoComplete();
        while(s && strncmp(s,strstart,l));
              
        return s;
    }
};
    

bool debugOnSignal=false;


const char *getPrompt(){
    extern Value promptCallback;
    
    static char buf[256];
    char pchar=0;
    if(a->isDefining())
        pchar = ':';
    else if(a->inSubContext())
        pchar = '*';
    else
        pchar = '>';
    
    if(promptCallback.t->isCallable()){
        a->pushInt(GarbageCollected::getGlobalCount());
        a->pushInt(a->stack.ct-1); // -1 because we just pushed the gcount
        buf[0]=pchar;
        buf[1]=0;
        a->pushString(buf);
        a->runValue(&promptCallback);
        strncpy(buf,a->popval()->toString().get(),256);
    } else {
        sprintf(buf,"%d|%d %c ",
                GarbageCollected::getGlobalCount(),
                a->stack.ct,pchar);
    }
    return buf;
}

void addDirToSearchPath(const char *data){
    char *path = realpath(data,NULL); // get absolute path
    *strrchr(path,'/')=0; // terminate at last / to get directory
    free((void *)a->appendToSearchPath(path));
    free((void *)path);
}

static EditLine *el=NULL;
static History *hist=NULL;

void cliShutdown(){
    if(el){
        completer::shutdown(el);
        history_end(hist);
        el_end(el);
        el=NULL;
    }
}

void sigh(int s){
    printf("Signal %d recvd, exiting\n",s);
    if(debugOnSignal){
        a->invokeDebugger();
    } else {
        cliShutdown();
        exit(1);
    }
}

int main(int argc,char *argv[]){
    signal(SIGSEGV,sigh);
    signal(SIGINT,sigh);
    
    extern void setArgumentList(int argc,char *argv[]);
    
    a = new Angort();
    
    // will register words - including %shutdown, which will run when
    // angort shuts down normally.
    a->registerLibrary(&LIBNAME(cli),true);
    
    // first, we'll try to include the standard startup
    try {
        a->include("angortrc",false);
    } catch(FileNotFoundException e){
        // ignore if not there
    } catch(Exception e){
        showException(e);
    }
    
    // create the stripped arg value.
    strippedArgs = Types::tList->set(&strippedArgVal);
    
    // this sets the RAW argument list; the argument list stripped of
    // angort-specific stuff is set below.
    setArgumentList(argc,argv);
    
    // set up debugger
    extern void basicDebugger(Angort *);
    a->setDebuggerHook(basicDebugger);
    
    int flags = 0;
    
    /*
       we can't use deep command line processing, because of
       how we might want to process arguments.
       Consider that the Angort programmer wants
       an option '-Zfilename', and the user enters '-Zdoofus." Unfortunately
       Angort will consume the 'd' thinking it's a debug option.
 
       The only option is to not permit option-chaining, so every option
       must be given its own dash. It's what Python does!
       
       What we CAN do is strip out the Angort-specific arguments,
       and the executable name. The first argument without a dash
       should always be the program name and this is easy to find.
     */
    
    int nonOptionArgs = 0;
    char *filename=NULL; // the filename (or -e string)
    // extra, for looping which has an "init" section.
    // We might do something like angort -n '0!Ct' '!+Ct ?Ct.'
    char *extradata = NULL; 
    for(int i=1;i<argc;i++){
        char *arg = argv[i];
        if(*arg == '-'){
            // it's an option.
            switch(arg[1]){
            case 'h':
                printf("Angort version %s %s\n",a->getVersion(),copyString);
                puts(usageString);
                exit(0);
                break;
            case '-':
                // copy ALL remaining arguments into stripped arg list,
                // ignoring them here
                for(i++;i<argc;i++)
                    Types::tString->set(strippedArgs->append(),argv[i]);
                break;
            case 'n':flags|=F_LOOP|F_CMD;break;
            case 'e':flags|=F_CMD;break;
            case 'd':a->debug|=1;break;
            case 'D':a->debug|=2;break;
            case 'b':debugOnSignal=true;break;
            case 'i':
                switch(arg[2]){
                case 'f':
                    a->importAllFuture();
                    break;
                case 'd':
                    a->importAllDeprecated();
                    break;
                default:
                    printf("should be -if or -id\n");
                    exit(1);
                }
                break;
            case 'l':
                a->plugin(arg+2);
                break;
            default:
                // unrecognised option
                Types::tString->set(strippedArgs->append(),argv[i]);
                break;
            }
        } else {
            // it's an non-option argument to copy over. If it's the
            // first one, set it as the filename.
            switch(nonOptionArgs){
            case 0 :  filename = argv[i];break;
            case 1:   extradata = argv[i];break;
            default:break;
            }
            nonOptionArgs++;
            Types::tString->set(strippedArgs->append(),argv[i]);
        }
    }
    
    // either the filename to run or a command (depending on opts)
    if(filename){
        try {
            if(flags & F_CMD){
                if(flags & F_LOOP){
                    if(!extradata){
                    printf("-n and -e require init and loop strings\n");
                    exit(1);
                    }
                    a->feed(filename); // do initial part
                    // looping is done with an egregious hack. Rather
                    // than muck around inside Angort, we actually
                    // fake up a function by feeding a colon definition
                    // into Angort. Then, for each line, we stack the
                    // line and then feed a line to run that function.
                    a->feed(":TMPLOOP");
                    a->feed(extradata);
                    a->feed(";");
                    // then iterate over stdin, executing this
                    // word for each line
                    while(!feof(stdin)){
                        char *buf=NULL;
                        size_t size;
                        int rv = getline(&buf,&size,stdin);
                        if(rv<=0)
                            break;
                        else{
                            buf[rv-1]=0;
                            a->pushString(buf);
                            a->feed("TMPLOOP");
                        }
                        if(buf)free(buf);
                    }
                } else 
                    a->feed(filename);
                exit(0);
            } else {
                FILE *f = fopen(filename,"r");
                if(!f){
                    printf("cannot open file: %s\n",filename);
                    exit(1);
                }
                addDirToSearchPath(filename);
                a->fileFeed(filename);
            }
        }catch(Exception e){
            showException(e);
        }
    }
    
    /*
     * 
     * After this point, editline is up so will need to be closed down on
     * exit. cliShutdown() will do this, and a normal angort shutdown will
     * do that because it will call %shutdown in the library, which will
     * call cliShutdown().
     */
    
    
    // then read lines from input
    
    a->assertDebug=true;
    printf("Angort version %s %s\nUse '??word' to get help on a word.\n",
           copyString,
           a->getVersion());
    
    // start up an editline instance
    
    el = el_init(argv[0],stdin,stdout,stderr);
    el_set(el,EL_PROMPT,&getPrompt);
    el_set(el,EL_EDITOR,"emacs");
    
    hist = history_init();
    HistEvent ev;
    history(hist,&ev,H_SETSIZE,800);
    el_set(el,EL_HIST,history,hist);
    if(!hist)
        printf("warning: no history\n");
        
    
    AngortAutocomplete comp;
    completer::setup(el,&comp,"\t\n\"\\'@><=;|&{(?! ");
    
    for(;;){
        // set up the autocomplete function and others
//        rl_completion_entry_function = autocomplete_generator;
//        rl_basic_word_break_characters = " \t\n\"\\'@><=;|&{(";
        
        int count;
        const char *line = el_gets(el,&count);
        // this avoids break happening when we exit the debugger
        // after a ctrl-c.
        extern bool debuggerBreakHack;
        if(debuggerBreakHack)
            debuggerBreakHack=false;
        else if(!line)
            break;

            
        
       
        if(count>1){ // there's going to be a trailing newline
            if(hist)
                history(hist,&ev,H_ENTER,line);
            try {
                // annoyingly, editline keeps any trailing newline
                char *tmp = strdup(line);
                if(*tmp && tmp[strlen(tmp)-1]=='\n')
                    tmp[strlen(tmp)-1]=0;
                a->feed(tmp);
                free(tmp);
            } catch(Exception e){
                showException(e);
            }
        }
    }
    
    delete a; // will call cli shutdown, which will close down el.
    return 0;
}
