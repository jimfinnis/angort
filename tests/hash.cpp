/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "test.h"
#include "hash.h"

#define HCOUNT 10000

class HashTest : public Test {
public:
    HashTest() : Test("Hash") {
        suite.add(this);
    }
    
    void t1(){
        printf("test 1, integers----\n");
        int keys[HCOUNT];
        int vals[HCOUNT];
        Hash h;
        Value v,w;
        
        for(int i=0;i<HCOUNT;i++){
            keys[i]=i*31;
            Types::tInteger->set(&v,keys[i]);
            vals[i]=rand()%100000;
            Types::tInteger->set(&w,vals[i]);
            h.set(&v,&w);
        }
        for(int i=0;i<HCOUNT;i++){
            Types::tInteger->set(&v,keys[i]);
            if(h.find(&v)){
                w.copy(h.getval());
                if(w.t != Types::tInteger)
                    die("value not an int");
                else if(w.toInt() != vals[i]){
                    printf("%d != %d\n",w.toInt(),vals[i]);
                    die("value mismatch");
                }
            } else
                die("key not found");
        }
    }
    
    void t2(){
        // string key checks
        
        printf("test 1, strings----\n");
        Hash h;
        Value v,w;
        char keys[HCOUNT][32];
        char vals[HCOUNT][32];
        
        for(int i=0;i<HCOUNT;i++){
            sprintf(keys[i],"foo%x",i*31);
            Types::tString->set(&v,keys[i]);
            sprintf(vals[i],"bar%d",rand()%100000);
            Types::tString->set(&w,vals[i]);
//            printf("%s %s\n",keys[i],vals[i]);
            h.set(&v,&w);
        }
        
        for(int i=0;i<HCOUNT;i++){
            char buf[32];
            Types::tString->set(&v,keys[i]);
            if(h.find(&v)){
                w.copy(h.getval());
                if(w.t != Types::tString)
                    die("value not an int");
                if(strcmp(w.toString(buf,32),vals[i])){
                    printf("%s != %s\n",w.toString(buf,32),vals[i]);
                    die("value mismatch");
                }
            } else
                die("key not found");
        }
    
    }
    
    virtual void run(Angort *a){
        
        Hash h;
        Value k,v;
        
        t1();
        t2();
    
    }
};
HashTest hash;
