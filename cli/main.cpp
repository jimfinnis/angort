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

struct MyProperty : public Property {
    int ctr;
    
    virtual void postSet(){
        printf("You've set the property to %f\n",v.toFloat());
    }
    
    virtual void preGet(){
        Types::tFloat->set(&v,(float)ctr++);
        printf("Property is about to be got.\n");
    }
};

int main(int argc,char *argv[]){
    Angort a;
    REGWORDS(a,stdmath);
    
    char buf[256];
    a.registerProperty("testpropertyignore",new MyProperty);
    
    if(argc>1){
        FILE *f = fopen(argv[1],"r");
        if(!f){
            printf("cannot open file: %s\n",argv[1]);
            exit(1);
        }
        uint32_t magic=0,version=0;
        fread(&magic,4,1,f);
        if(magic==ANGORT_MAGIC){
            fread(&version,4,1,f);
            fclose(f);
            if(version!=ANGORT_VERSION){
                printf("image file version incorrect: %d\n",version);
                exit(1);
            }
            try{
                a.loadImage(argv[1]);
            }catch(Exception e){
                printf("Error: %s\n",e.what());
                exit(1);
            }
                
        }else{
            // attempt to read as a script file
            fclose(f);
            a.fileFeed(argv[1]);
        }
    }
    
    a.assertDebug=true;
    
    printf("Angort version %d.%d (c) Jim Finnis 2014\nUse 'help \"word\"' to get help on a word.\n",
           ANGORT_VERSION / 100,
           ANGORT_VERSION % 100);
    
    for(;;){
        char prompt=0;
        if(a.defining)
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
            try{
                a.feed(line);
            } catch(Exception e){
                printf("Error: %s\n",e.what());
                a.clearStack();
            }
        }
    }
}
