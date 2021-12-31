/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */
#include "config.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#if !NOLINEEDITING
#include <histedit.h>
#include "completer.h"
#endif

#include "angort.h"


// keep this up to date!
static const char *copyString="(c) Jim Finnis 2012-2020";
static const char *usageString=
"\nUsage: angort [-?] [-n] [-e] [-d] [-D] [-b] [-if] [-in]\n"
"        [-llib] [-llib]..\n\n"
"-?    : this string\n"
"-n    : execute command-line script in loop, requires two args: init\n"
"        and loop (the latter reads lines from stdin)\n"
"-e    : execute command-line script\n"
"-d    : disassemble while running\n"
"-D    : tokeniser trace\n"
"-L    : print input lines\n"
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

Runtime *runtime; // default angort runtime

static void showException(Exception& e){
    WriteLock lock=WL(&globalLock);
    printf("Error in thread %d: %s\n",e.run?e.run->id:-1,e.what());
    if(e.ip){
        printf("Error at:");
        runtime->showop(e.ip);
        printf("\n");
    }else
          printf("Last line input: %s\n",runtime->ang->getLastLine());
    runtime->clearStack();
    if(e.fatal)
        exit(1);
    
}

#define F_CMD 1
#define F_LOOP 2
#define F_FUTURE 4
#define F_DEPRECATED 8

#if !NOLINEEDITING
// autocompletion data generator
class AngortAutocomplete : public completer::Iterator {
    const char *strstart;
    int l;
public:
    virtual void first(const char *stringstart, int len){
        strstart = stringstart;
        l = len;
        runtime->ang->resetAutoComplete();
    }
    virtual const char *next(){
        const char *s=NULL;
        do
            s = runtime->ang->getNextAutoComplete();
        while(s && strncmp(s,strstart,l));
              
        return s;
    }
};
#endif
    

bool debugOnSignal=false;

// this symbol needs to be defined if editline wasn't compiled with
// UNICODE support, as it wasn't on earlier versions of Ubuntu.

#if(EDITLINE_NOUNICODE)
const char *getPrompt(){
    extern Value promptCallback;
#else
const wchar_t *getPrompt(){
    extern Value promptCallback;
#endif
    static char buf[256];
    char pchar=0;
    if(runtime->ang->isDefining())
        pchar = ':';
    else if(runtime->ang->inSubContext())
        pchar = '*';
    else
        pchar = '>';
    
    if(promptCallback.t->isCallable()){
        runtime->pushInt(GarbageCollected::getGlobalCount());
        runtime->pushInt(runtime->stack.ct-1); // -1 because we just pushed the gcount
        buf[0]=pchar;
        buf[1]=0;
        runtime->pushString(buf);
        runtime->runValue(&promptCallback);
        strncpy(buf,runtime->popval()->toString().get(),256);
    } else {
        sprintf(buf,"%d|%d %c ",
                GarbageCollected::getGlobalCount(),
                runtime->stack.ct,pchar);
    }
#if(EDITLINE_NOUNICODE)
    return buf;
#else
    // convert to wide, we have to do it here rather than leave it
    // to EditLine because there is a bug in EditLine's prompt code
    // (if conversion fails, a NULL is returned which print_prompt()
    // cheerfully tries to print).
    static wchar_t wbuf[256];
failed:
    int conv = mbstowcs(wbuf,buf,256);
    if(conv != strlen(buf)){
        strcpy(buf,"(prompt contains bad unicode) > "); 
        goto failed;
    }
    return wbuf;
#endif
}

void addDirToSearchPath(const char *data){
    char *path = realpath(data,NULL); // get absolute path
    *strrchr(path,'/')=0; // terminate at last / to get directory
    free((void *)runtime->ang->appendToSearchPath(path));
    free((void *)path);
}

#if !NOLINEEDITING
static EditLine *el=NULL;
static History *hist=NULL;
#endif

void cliShutdown(){
#if !NOLINEEDITING
    if(el){
        completer::shutdown(el);
        history_end(hist);
        el_end(el);
        el=NULL;
    }
#endif
}

void cliSighandler(int s){
    //todothread - make this happen in the appropriate thread!
    printf("Signal %d recvd, exiting\n",s);
    if(debugOnSignal && runtime->ang->debuggerHook){
        (*runtime->ang->debuggerHook)(runtime);
    } else {
        cliShutdown();
        exit(1);
    }
}

int main(int argc,char *argv[]){
    
    struct sigaction sa;
    sa.sa_handler = cliSighandler;
    memset(&sa,0,sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV,&sa,NULL);
    sigaction(SIGINT,&sa,NULL);
    
    extern void setArgumentList(int argc,char *argv[]);
    
    Angort *a = new Angort();
    runtime = a->run;
    
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
    extern void basicDebugger(Runtime *);
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
            case '?':
                printf("Angort version %s %s\n",a->getVersion(),copyString);
                puts(usageString);
                exit(0);
                break;
            case '-':
                // If just a '--' with no following chars,
                // copy ALL remaining arguments into stripped arg list,
                // ignoring them here. Otherwise add to the args list.
                
                if(!arg[2]){
                    for(i++;i<argc;i++)
                        Types::tString->set(strippedArgs->append(),argv[i]);
                } else 
                    Types::tString->set(strippedArgs->append(),argv[i]);
                    
                break;
            case 'n':flags|=F_LOOP|F_CMD;break;
            case 'e':flags|=F_CMD;break;
            case 'd':runtime->trace=true;break;
            case 'D':a->tokeniserTrace=true;break;
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
            case 'L':
                a->printLines = true;
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
                            runtime->pushString(buf);
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
    
    runtime->assertDebug=true;
    fprintf(stderr,"Angort version %s %s\nUse '??word' to get help on a word.\n",
           copyString,
           a->getVersion());
    
    // start up an editline instance
    
#if !NOLINEEDITING
    el = el_init(argv[0],stdin,stdout,stderr);
#if(EDITLINE_NOUNICODE)
    el_set(el,EL_PROMPT,&getPrompt); // sorry, no unicode support...
#else
    el_wset(el,EL_PROMPT,&getPrompt); // use wide prompt (see getPrompt for why)
#endif
    el_set(el,EL_EDITOR,"emacs");
    hist = history_init();
    HistEvent ev;
    history(hist,&ev,H_SETSIZE,800);
    el_set(el,EL_HIST,history,hist);
    if(!hist)
        printf("warning: no history\n");
    AngortAutocomplete comp;
    completer::setup(el,&comp,"\t\n\"\\'@><=;|&{(?! ");
#endif
    
        
    for(;;){
        // set up the autocomplete function and others
//        rl_completion_entry_function = autocomplete_generator;
//        rl_basic_word_break_characters = " \t\n\"\\'@><=;|&{(";
        
        int count;
#if !NOLINEEDITING
        const char *line = el_gets(el,&count);
        
#else
#if(EDITLINE_NOUNICODE)
        fputs(getPrompt(),stdout);
#else
        fputws(getPrompt(),stdout);
#endif
        char inbuf[1024];
        const char *line = fgets(inbuf,1024,stdin);
        if(line)
            count=strlen(line);
        else
            count=0;
#endif        
        // this avoids break happening when we exit the debugger
        // after a ctrl-c.
        extern bool debuggerBreakHack;
        if(debuggerBreakHack)
            debuggerBreakHack=false;
        else if(!line)
            break;
        
       
        if(count>1){ // there's going to be a trailing newline
#if !NOLINEEDITING
            if(hist)
                history(hist,&ev,H_ENTER,line);
#endif
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
