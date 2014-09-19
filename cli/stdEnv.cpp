#include "angort.h"

using namespace angort;


static Value argList;

void setArgumentList(int argc,char *argv[]){
    ArrayList<Value>*list = Types::tList->set(&argList);
    
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

%word getenv (name -- string/none)
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
