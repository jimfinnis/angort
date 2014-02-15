/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#include "angort.h"
#include "ser.h"
#include "hash.h"

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

%word remove (idx list -- item) remove an item by index, returning it
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    int i = a->popInt();
    
    Value *v = a->pushval();
    v->copy(list->get(i));
    list->remove(i);
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


%word map (iter func -- list) apply a function to an iterable, giving a list
{
    Value func;
    func.copy(a->popval()); // need a local copy
    Value *iterable = a->popval();
    
    Iterator<Value *> *iter = iterable->t->makeValueIterator(iterable);
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current());
        a->runValue(&func);
        Value *v = list->append();
        v->copy(a->popval());
    }
    delete iter;
}

%word inject (start iter func -- result) see Ruby's docs on this :)
{
    Value func;
    func.copy(a->popval()); // need a local copy
    
    Value *iterable = a->popval();
    Iterator<Value *> *iter = iterable->t->makeValueIterator(iterable);
    
    // accumulator is already on the stack
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current()); // stack the iterator on top of the accum
        a->runValue(&func); // run the function, leaving the new accumulator
    }
    delete iter;
}

%word hash (-- hash) create a new hash
{
    Value *v = a->pushval();
    Types::tHash->set(v);
}

%word hset (val key hash --) set a value in a hash
{
    Hash *h = Types::tHash->get(a->popval());
    Value *k = a->popval();
    Value *v = a->popval();
    
    h->set(k,v);
    
}

%word hget (key hash --) get a value in a hash, or tNone
{
    Hash *h = Types::tHash->get(a->popval());
    Value *k = a->stack.peekptr();
    if(h->find(k))
        k->copy(h->getval());
    else 
        k->clr();
}

