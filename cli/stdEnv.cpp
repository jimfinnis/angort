#include "angort.h"

using namespace angort;


static Value argList;

// we store this because in certain plugins the arglist is useful.

namespace angort {
char **sysArgv;
int sysArgc;
};

void setArgumentList(int argc,char *argv[]){
    ArrayList<Value>*list = Types::tList->set(&argList);
    
    sysArgc=argc;
    sysArgv=argv;
    
    for(int i=0;i<argc;i++){
        Value *v = list->append();
        Types::tString->set(v,argv[i]);
    }
}
              
%name stdenv

%word args (-- list) get command line arguments
{
    Value *v = a->pushval();
    v->copy(&argList);
}

%word getenv (name -- string/none) get environment variable
{
    Value *p;
    a->popParams(&p,"s");
    
    const char *s = getenv(p->toString().get());
    p = a->pushval();
    
    if(s)
        Types::tString->set(p,s);
    else
        p->setNone();
}


%word setenv (value name --) set environment variable
{
    Value *p[2];
    a->popParams(p,"ss");
    setenv(p[1]->toString().get(),p[0]->toString().get(),1);
}
