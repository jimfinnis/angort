/**
 * @file
 * Lists and hashes
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

%word get (key coll --) get an item from a list or hash
{
    Value *c = a->popval();
    Value *keyAndResult = a->stack.peekptr();
    Value v;
    c->t->getValue(c,keyAndResult,&v);
    keyAndResult->copy(&v); // copy into the key's slot
}
%word set (val key coll --) put an item into a list or hash
{
    Value *c = a->popval();
    Value *k = a->popval();
    Value *v = a->popval();
    c->t->setValue(c,k,v);
}
%word count (list --) get count
{
    Value *c = a->stack.peekptr();
    int ct = c->t->getCount(c);
    Types::tInteger->set(c,ct);
}

%word remove (idx list -- item) remove an item by index, returning it
{
    Value *c = a->popval();
    Value *keyAndResult = a->stack.peekptr();
    Value v;
    c->t->removeAndReturn(c,keyAndResult,&v);
    keyAndResult->copy(&v); // copy into the key's slot
}


%word shift (list -- item) remove and return the first item of the list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->pushval();
    
    v->copy(list->get(0));
    list->remove(0);
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
    Value *src = list->get(list->count()-1);
    v->copy(src);
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
    
    Iterator<Value *> *iter = iterable->t->makeIterator(iterable);
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current());
        a->runValue(&func);
        Value *v = list->append();
        v->copy(a->popval());
    }
    delete iter;
}

%word reduce (start iter func -- result) perform a (left) fold or reduce on an iterable
{
    Value func;
    func.copy(a->popval()); // need a local copy
    
    Value *iterable = a->popval();
    Iterator<Value *> *iter = iterable->t->makeIterator(iterable);
    
    // accumulator is already on the stack
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current()); // stack the iterator on top of the accum
        a->runValue(&func); // run the function, leaving the new accumulator
    }
    delete iter;
}

%word filter (iter func -- list) filter an iterable with a boolean function
{
    Value func;
    func.copy(a->popval()); // need a local copy
    Value *iterable = a->popval();
    
    Iterator<Value *> *iter = iterable->t->makeIterator(iterable);
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current());
        a->runValue(&func);
        if(a->popval()->toInt()){
            Value *v = list->append();
            v->copy(iter->current());
        }
    }
    delete iter;
}

%word in (item iterable -- bool) return if item is in list or hash keys
{
    Value *iterable = a->popval();
    Value *item = a->popval();
    
    a->pushInt(iterable->t->isIn(iterable,item)?true:false);
}
