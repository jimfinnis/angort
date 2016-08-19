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

%wordargs head li (coll n -- list) get the first n items of a list
{
    Value r;
    ArrayList<Value> *list = Types::tList->set(&r);
    for(int i=0;i<p1;i++){
        if(i==p0->count())break;
        list->append()->copy(p0->get(i));
    }
    a->pushval()->copy(&r);
}

%wordargs tail li (coll n -- list) get the last n items of a list
{
    Value r;
    ArrayList<Value> *list = Types::tList->set(&r);
    int start = p0->count()-p1;
    if(start<0)start=0;
    for(int i=start;i<p0->count();i++){
        list->append()->copy(p0->get(i));
    }
    a->pushval()->copy(&r);
}

%wordargs splitlist li (coll n -- [list,list]) split list into two lists of [n,count-n] items
{
    Value r;
    ArrayList<Value> *list = Types::tList->set(&r);
    ArrayList<Value> *list1 = Types::tList->set(list->append());
    ArrayList<Value> *list2 = Types::tList->set(list->append());
    
    for(int i=0;i<p0->count();i++){
        (i<p1 ? list1 : list2)->append()->copy(p0->get(i));
    }
    a->pushval()->copy(&r);
}
    

%word last (coll -- item/none) get last item
{
    Value *c = a->stack.peekptr();
    int n = c->t->getCount(c)-1;
    if(n<0)
        c->clr();
    else {
        Value v,out;
        Types::tInteger->set(&v,n);
        c->t->getValue(c,&v,&out);
        c->copy(&out);
    }
}

inline void getByIndex(Value *c,int idx){
    if(idx>=c->t->getCount(c))
        c->clr();
    else {
        Value v,out;
        Types::tInteger->set(&v,idx);
        c->t->getValue(c,&v,&out);
        c->copy(&out);
    }
}

%word explode ([x,y,z] -- z y x) put all items onto stack 
{
    Value *iterable = a->popval();
    Iterator<Value *> *iter = iterable->t->makeIterator(iterable);
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current());
    }
    delete iter;
}    

%word fst (coll -- item/none) get first item
{
    getByIndex(a->stack.peekptr(),0);
}

%word snd (coll -- item/none) get second item
{
    getByIndex(a->stack.peekptr(),1);
}
%word third (coll -- item/none) get second item
{
    getByIndex(a->stack.peekptr(),2);
}
%word fourth (coll -- item/none) get second item
{
    getByIndex(a->stack.peekptr(),3);
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
    Value v;
    v.copy(a->popval());
    ArrayList<Value> *list = Types::tList->get(&v);
    
    Value *p = a->pushval();
    Value *src = list->get(list->count()-1);
    p->copy(src);
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

%word filter2 (iter func -- falselist truelist) filter an iterable with a boolean function into two lists
{
    Value func;
    func.copy(a->popval()); // need a local copy
    Value *iterable = a->popval();
    
    Iterator<Value *> *iter = iterable->t->makeIterator(iterable);
    ArrayList<Value> *falselist = Types::tList->set(a->pushval());
    ArrayList<Value> *truelist = Types::tList->set(a->pushval());
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current());
        a->runValue(&func);
        Value *v;
        if(a->popval()->toInt())
            v = truelist->append();
       else
            v = falselist->append();
        v->copy(iter->current());
            
    }
    delete iter;
}

%word in (item iterable -- bool) return if item is in list or hash keys
{
    Value *iterable = a->popval();
    Value *item = a->popval();
    
    bool b = iterable->t->getIndexOfContainedItem(iterable,item)>=0;
    
    a->pushInt(b?1:0);
}

%word index (item iterable -- int) return index of item in iterable, or none
{
    Value *iterable = a->popval();
    Value *item = a->popval();
    
    int i = iterable->t->getIndexOfContainedItem(iterable,item);
    if(i<0)a->pushNone();
    else a->pushInt(i);
}

%word slice (iterable start len -- iterable) produce a slice of a string or list
{
    int len = a->popInt();
    int start = a->popInt();
    Value iterable;
    iterable.copy(a->popval());
    
    Value *res = a->pushval();
    iterable.t->slice(res,&iterable,start,len);
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

%word zipWith (in1 in2 func -- out) apply binary func to pairs of items in list
{
    Value *p[3];
    a->popParams(p,"IIc");
    Iterator<Value *> *iter1 = p[0]->t->makeIterator(p[0]);
    Iterator<Value *> *iter2 = p[1]->t->makeIterator(p[1]);
    
    Value func;
    func.copy(p[2]); // need a local copy
    
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    for(iter1->first(),iter2->first();
        !(iter1->isDone() || iter2->isDone());
        iter1->next(),iter2->next()){
        
        a->pushval()->copy(iter1->current());
        a->pushval()->copy(iter2->current());
        a->runValue(&func);
        
        list->append()->copy(a->popval());
    }
    
    delete iter1;
    delete iter2;
    
}

%word reverse (iter -- list) reverse an iterable to return a list
{
    Value *p = a->popval();
    Iterator<Value *> *iter = p->t->makeIterator(p);
    
    // quickest thing to do is count the items by hand. This
    // is optimised if we know the input iterable is a list.
    
    int n;
    if(p->t == Types::tList){
        n = Types::tList->get(p)->count();
    } else {
        n=0;
        for(iter->first();!iter->isDone();iter->next())n++;
    }
    
    // new list
    ArrayList<Value> *list = Types::tList->set(a->pushval());
    
    int i=n;
    for(iter->first();!iter->isDone();iter->next()){
        Value *v = iter->current();
        list->set(--i,v);
    }
    delete iter;
    
}



%word intercalate (iter string -- string) turn elements of collection into string and intercalate with a separator
{
    Value *p[2];
    a->popParams(p,"vv"); // can be any type
    
    Value *v = p[0];
    Iterator<Value *> *iter = v->t->makeIterator(v);
    const StringBuffer& s = p[1]->toString();
    const char *sep = s.get();
    int seplen = strlen(sep);
    
    int count = v->t->getCount(v);
    
    // first pass to get the lengths
    int len=0;
    int n=0;
    for(n=0,iter->first();!iter->isDone();iter->next(),n++){
        len += strlen(iter->current()->toString().get());
        if(n!=count-1)
            len += seplen;
    }
    
    // allocate the result in a new string value on the stack
    char *out = Types::tString->allocate(a->pushval(),len+1,Types::tString);
    // second pass to write the value
    *out = 0;
    for(n=0,iter->first();!iter->isDone();iter->next(),n++){
        const StringBuffer& b = iter->current()->toString();
        strcpy(out,b.get());
        out += strlen(b.get());
        if(n!=count-1){
            strcpy(out,sep);
            out+=seplen;
        }
    }
    delete iter;
}


%word listintercalate (iter item -- list) intercalate items in a list with another item
{
    Value *p[2];
    a->popParams(p,"vv"); // can be any type
    
    Value *v = p[0];
    Iterator<Value *> *iter = v->t->makeIterator(v);
    
    Value sep,output;
    sep.copy(p[1]);
    ArrayList<Value> *list = Types::tList->set(&output);
    
    int count = v->t->getCount(v);
    int n=0;
    for(iter->first();!iter->isDone();iter->next(),n++){
        list->append()->copy(iter->current());
        if(n!=count-1){
            list->append()->copy(&sep);
        }
    }
    delete iter;
    
    a->pushval()->copy(&output);
    output.clr();
}
