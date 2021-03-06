/**
 * \file
 * Brief description. Longer description.
 * 
 * \author $Author$
 * \date $Date$
 */


#ifndef __TEST_H
#define __TEST_H

#include <stdio.h>
#include "angort.h"

#include <vector>
using namespace std;
using namespace angort;

extern Angort *newAngort();

class Test {
protected:
    const char *name;
public:
    Test(const char *_name){
        name = strdup(_name);
    }
    const char *getName(){
        return name;
    }
    
    void die(const char *s){
        throw AssertException(s,0);
    }
    
    virtual void run(Angort *a)=0;
};

class TestSuite {
    vector<Test *>tests;
    const char *name;
    Test *cur;
public:
    TestSuite(const char *_name){
        name = strdup(_name);
    }
    
    void add(Test *t){
        tests.push_back(t);
    }
    
    int getCount(){
        return tests.size();
    }
    

    int run(){
        printf("Test suite: %s\n",name);
        int totalOK=0;
        vector<Test *>::const_iterator cii;
        for(cii=tests.begin();cii!=tests.end();cii++){
            cur = *cii;
            try {
                Angort *a = newAngort();
                cur->run(a);
                delete a;
                printf("OK    :%20s\n",cur->getName());
                totalOK++;
            } catch (Exception e){
                printf("FAILED: %20s : %s\n",cur->getName(),
                       e.what());
            }
        }
        return totalOK;
    }
};

extern TestSuite suite;

#endif /* __TEST_H */
