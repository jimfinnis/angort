/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "angort.h"

WORDS(stdmath)

static void showException(Exception& e,Angort& a){
    printf("Error: %s\n",e.what());
    const Instruction *ip = a.getIPException();
    if(ip){
        char buf[1024];
        ip->getDetails(buf,1024);
        printf("Error at %s\n",buf);
    }
    
    a.clearStack();
}

int main(int argc,char *argv[]){
    Angort a;
    REGWORDS(a,stdmath);
    
    // first, we'll try to include the standard startup
    try {
        a.include(".angortrc",false);
    } catch(FileNotFoundException e){
    } catch(Exception e){
        showException(e,a);
    }
    
    // then any file on the command line
    char buf[256];
    if(argc>1){
        FILE *f = fopen(argv[1],"r");
        if(!f){
            printf("cannot open file: %s\n",argv[1]);
            exit(1);
        }
        
        try{
            a.fileFeed(argv[1]);
        }catch(Exception e){
            showException(e,a);
        }
    }
    
    a.assertDebug=true;
    int vv = a.getVersion();
    printf("Angort version %d.%d (c) Jim Finnis 2014\nUse '\"word\" help' to get help on a word.\n",
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
        return 0;
    }
}
