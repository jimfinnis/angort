/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "angort.h"

using namespace angort;

static void showException(Exception& e,Angort& a){
    printf("Error: %s\n",e.what());
    const Instruction *ip = a.getIPException();
    if(ip){
        char buf[1024];
        ip->getDetails(buf,1024);
        printf("Error at %s\n",buf);
    }else
          printf("Last line input: %s\n",a.getLastLine());
    if(e.fatal)
        exit(1);
    
    a.clearStack();
}

#define F_CMD 1
#define F_LOOP 2


int main(int argc,char *argv[]){
    Angort a;
    
    extern void setArgumentList(int argc,char *argv[]);
    extern LibraryDef LIBNAME(stdmath);
    extern LibraryDef LIBNAME(stdenv);
    
    // first, we'll try to include the standard startup
    try {
        a.registerLibrary(&LIBNAME(stdmath),true);
        a.registerLibrary(&LIBNAME(stdenv),true);
        a.include("angortrc",false);
    } catch(FileNotFoundException e){
        // ignore if not there
    } catch(Exception e){
        showException(e,a);
    }
    
    setArgumentList(argc,argv);
    
    int flags = 0;
    char c;
    while((c=getopt(argc,argv,"endDl:"))!=-1){
        switch(c){
        case 'n':flags|=F_LOOP;break;
        case 'e':flags|=F_CMD;break;
        case 'd':a.debug|=1;break;
        case 'D':a.debug|=2;break;
        case 'l':
            printf("%p: %s\n",optarg,optarg);
            a.plugin(optarg);
            break;
        default:
            break;
        }
    }
    
    // either the filename to run or a command (depending on opts)
    if(argc>optind){
            const char *data = argv[optind];
        try {
            if(flags & F_CMD){
                if(flags & F_LOOP){
                    // looping is done with an egregious hack. Rather
                    // than muck around inside Angort, we actually
                    // fake up a function by feeding a colon definition
                    // into Angort. Then, for each line, we stack the
                    // line and then feed a line to run that function.
                    a.feed(":TMPLOOP");
                    a.feed(data);
                    a.feed(";");
                    // then iterate over the file, executing this
                    // word for each line
                    while(!feof(stdin)){
                        char *buf=NULL;
                        size_t size;
                        int rv = getline(&buf,&size,stdin);
                        if(rv<=0)
                            break;
                        else{
                            buf[rv-1]=0;
                            a.pushString(buf);
                            a.feed("TMPLOOP");
                        }
                        if(buf)free(buf);
                    }
                } else 
                    a.feed(data);
                exit(0);
            } else {
                FILE *f = fopen(data,"r");
                if(!f){
                    printf("cannot open file: %s\n",data);
                    exit(1);
                }
                a.fileFeed(data);
            }
        }catch(Exception e){
            showException(e,a);
        }
    }
    
    // then read lines from input
    
    char buf[256];
    
    a.assertDebug=true;
    int vv = a.getVersion();
    printf("Angort version %d.%d (c) Jim Finnis 2012-2015\nUse '??word' to get help on a word.\n",
           vv / 100,
           vv % 100);
    
    for(;;){
        char prompt=0;
        if(a.isDefining())
            prompt = ':';
        else if(a.inSubContext())
            prompt = '*';
        else
            prompt = '>';
        
        sprintf(buf,"%d|%d %c ",
                GarbageCollected::getGlobalCount(),
                a.stack.ct,prompt);
        char *line = readline(buf);
        if(!line)break;
        if(*line){
            add_history(line);
            try {
                a.feed(line);
            } catch(Exception e){
                showException(e,a);
            }
        }
    }
    return 0;
}
