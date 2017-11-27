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
#include <readline/readline.h>
#include <readline/history.h>

#include "angort.h"

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

static char *autocomplete_generator(const char *text,int state){
    static int idx;
    const char *name;
    static int len;
    const char *start = text; // record this for prefixes
    
    // skip any the number of ? or ! at the beginning
    // so ??word and ?/! var will work. I would use
    // rl_special_prefixes, but it doesn't seem to behave.
    
    while(*text && (*text=='!' ||*text=='?'))text++;
    
    if(!state){
        len=strlen(text);
        a->resetAutoComplete();
    }
    
    // return the next name which partially matches
    while(name = a->getNextAutoComplete()){
        idx++;
        if(!strncmp(name,text,len)){
            if(start!=text){
                char *ss = (char *)
                      malloc(strlen(name)+(text-start)+1);
                memcpy(ss,start,text-start);
                strcpy(ss+(text-start),name);
                free((void *)name);
                return ss;
            }
            return (char *)name;
        }
    }
    return NULL;
}

const char *getPrompt(Angort *a){
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

void sigh(int s){
    printf("Signal %d recvd, exiting\n",s);
    exit(1);
}

int main(int argc,char *argv[]){
    signal(SIGSEGV,sigh);
    
    extern void setArgumentList(int argc,char *argv[]);
    
    a = new Angort();
    
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
    
    // set up debugger
    extern void basicDebugger(Angort *);
    a->setDebuggerHook(basicDebugger);
    
    // then read lines from input
    
    a->assertDebug=true;
    printf("Angort version %s (c) Jim Finnis 2012-2017\nUse '??word' to get help on a word.\n",
           a->getVersion());
    
    for(;;){
        const char *prompt = getPrompt(a);
        
        // set up the autocomplete function and others
        rl_completion_entry_function = autocomplete_generator;
        rl_basic_word_break_characters = " \t\n\"\\'@><=;|&{(";
        
        char *line = readline(prompt);
        if(!line)break;
        if(*line){
            add_history(line);
            try {
                a->feed(line);
            } catch(Exception e){
                showException(e);
            }
        }
    }
    delete a;
    return 0;
}
