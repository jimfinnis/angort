/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"
#include "ser.h"

%name lists

%word dumplist (list --) Dump a list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    
    static char buf[1024];
    for(int i=0;i<list->count();i++){
        const char *s = list->get(i)->toString(buf,1024);
        printf("%d: %s\n",i,s);
    }
}

%word get (idx list --) get an item from a list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    int idx = a->popInt();
    
    Value *v = a->pushval();
    v->copy(list->get(idx));
}
%word put (val idx list --) put an item into a list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    int idx = a->popInt();
    Value *v = a->popval();
    list->set(idx,v);
}
%word count (list --) get count
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->pushval();
    Types::tInteger->set(v,list->count());
}

%word remove (idx list --) remove an item by index
{
}


%word shift (list -- item) remove and return the first item of the list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->pushval();
    
    v->copy(list->get(list->count()-1));
    list->remove(list->count()-1);
}

%word unshift (item list --) prepend an item to a list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->popval();
    list->insert(0)->copy(v);
}

%word pop (list -- item) pop an item from the end of the list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->pushval();
    v->copy(list->get(list->count()-1));
    list->remove(list->count()-1);
}


%word push (item list --) append an item to a list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->popval();
    list->append()->copy(v);
}

