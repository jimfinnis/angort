/**
 * @file
 * Lists and hashes
 * 
 */

#include "angort.h"
#include "hash.h"
#include "opcodes.h"

using namespace angort;

namespace angort {
struct RevStdComparator : public ArrayListComparator<Value> {
    Angort *ang;
    RevStdComparator(Angort *a){
        ang = a;
    }
    virtual int compare(const Value *a, const Value *b){
        // binop isn't const, sadly.
        ang->binop(const_cast<Value *>(b),
                   const_cast<Value *>(a),OP_CMP);
        return ang->popInt();
    }
};

struct FuncComparator : public ArrayListComparator<Value> {
    Value *func;
    Angort *ang;
    
    FuncComparator(Angort *a,Value *f){
        ang = a;
        func = f;
    }
    virtual int compare(const Value *a, const Value *b){
        ang->pushval()->copy(a);
        ang->pushval()->copy(b);
        ang->runValue(func);
        return ang->popInt();
    }
};

}

%name coll

%word dumplist (list --) Dump a list
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    
    for(int i=0;i<list->count();i++){
        const StringBuffer& s = list->get(i)->toString();
        printf("%d: %s\n",i,s.get());
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
%word len (list --) get length of list, hash or string
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

%word slice (start len iterable -- iterable) produce a slice of a string or list
{
    Value *iterable = a->popval();
    int len = a->popInt();
    int start = a->popInt();
    
    Value *res = a->pushval();
    iterable->t->slice(res,iterable,start,len);
}

%word clone (in -- out) construct a shallow copy of a collection
{
    Value *v = a->stack.peekptr();
    v->t->clone(v,v,false);
}

%word deepclone (in -- out) produce a deep copy of a value
{
    Value *v = a->stack.peekptr();
    v->t->clone(v,v,true);
    
}

struct StdComparator : public ArrayListComparator<Value> {
    Angort *ang;
    StdComparator(Angort *a){
        ang = a;
    }
    virtual int compare(const Value *a, const Value *b){
        // binop isn't const, sadly.
        ang->binop(const_cast<Value *>(a),
                   const_cast<Value *>(b),OP_CMP);
        return ang->popInt();
    }
};

%word sort (in --) sort a list in place using default comparator
{
    Value listv;
    // need copy because comparators use the stack
    listv.copy(a->popval());
    ArrayList<Value> *list = Types::tList->get(&listv);
    
    StdComparator cmp(a);
    list->sort(&cmp);
}

%word rsort (in --) reverse sort a list in place using default comparator
{
    Value listv;
    // need copy because comparators use the stack
    listv.copy(a->popval());
    ArrayList<Value> *list = Types::tList->get(&listv);
    
    RevStdComparator cmp(a);
    list->sort(&cmp);
}


%word fsort (in func --) sort a list in place using function comparator
{
    Value func,listv;
    
    // need copies because comparators use the stack
    func.copy(a->popval());
    listv.copy(a->popval());
    
    ArrayList<Value> *list = Types::tList->get(&listv);
    
    FuncComparator cmp(a,&func);
    list->sort(&cmp);
}

%word all (in func --) true if the function returns true for all items
{
    int rv=1; // true by default
    
    Value func;
    func.copy(a->popval()); // need a local copy
    
    Value *iterable = a->popval();
    Iterator<Value *> *iter = iterable->t->makeIterator(iterable);
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current()); // stack the iterator on top of the accum
        a->runValue(&func); // run the function
        if(!a->popInt()){
            rv = 0;
            break;
        }
    }
    a->pushInt(rv);
    delete iter;
}

%word any (in func --) true if the function returns true for all items
{
    int rv=0; // false by default
    
    Value func;
    func.copy(a->popval()); // need a local copy
    
    Value *iterable = a->popval();
    Iterator<Value *> *iter = iterable->t->makeIterator(iterable);
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current()); // stack the iterator on top of the accum
        a->runValue(&func); // run the function
        if(a->popInt()){
            rv = 1;
            break;
        }
    }
    a->pushInt(rv);
    delete iter;
}
