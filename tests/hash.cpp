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
    
    virtual void run(Angort *a){
        printf("----\n");
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
};
HashTest hash;
