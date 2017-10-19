/**
 * @file
 * Lists and hashes
 * 
 */

#include "angort.h"
#include "hash.h"
#include "opcodes.h"

#include <wchar.h>
#include <wctype.h>

inline int wstrlen(const char *s){
    return mbstowcs(NULL,s,0);
}


using namespace angort;

namespace angort {
// DESTRUCTIVE comparator - damages the stack!
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

// DESTRUCTIVE comparator - damages the stack!
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


// DESTRUCTIVE comparator - damages the stack!
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
Write the contents of a list crudely to stdout. Consider using
util$show instead, from the util package.
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    
    for(int i=0;i<list->count();i++){
        const StringBuffer& s = list->get(i)->toString();
        printf("%d: %s\n",i,s.get());
    }
}

%wordargs head vi (coll/str n -- list) get the first n items/chars of a list or string
If a list, get the first n items into a new list - returns a list even if the length
requested is 1. (See also "fst".) In the case of a string, returns the first n characters.
{
    if(p0->t == Types::tList){
        ArrayList<Value> *in = Types::tList->get(p0);
        Value r;
        ArrayList<Value> *list = Types::tList->set(&r);
        for(int i=0;i<p1;i++){
            if(i==in->count())break;
            list->append()->copy(in->get(i));
        }
        a->pushval()->copy(&r);
    } else {
        const StringBuffer &b = p0->toString();
        const char *strin = b.get();
        
        if(p1<=0){
            a->pushString("");
            return;
        }
        int len = wstrlen(strin);
        
        // this is how many chars we convert
        p1 = p1<len ? p1 : len;
        
        // convert that many characters
        wchar_t *s = (wchar_t *)alloca((p1+1)*sizeof(wchar_t));
        int rv = mbstowcs(s,strin,p1+1);
        if(rv<0){
            a->pushNone();
            return;
        }
        // terminate
        s[p1]=0;
        
        // now convert back
        int l2 = wcstombs(NULL,s,0)+1;
        char *s2 = (char *)alloca(l2);
        wcstombs(s2,s,l2);
        a->pushString(s2);
    }
}

%wordargs tail vi (coll n -- list) get the last n items of a list
If a list, get the last n items into a new list - returns a list even if the length
requested is 1. (See also "last".) In the case of a string, returns the last n chars.
{
    if(p0->t == Types::tList){
        ArrayList<Value> *in = Types::tList->get(p0);
        Value r;
        ArrayList<Value> *list = Types::tList->set(&r);
        int start = in->count()-p1;
        if(start<0)start=0;
        for(int i=start;i<in->count();i++){
            list->append()->copy(in->get(i));
        }
        a->pushval()->copy(&r);
    } else {
        const StringBuffer &b = p0->toString();
        const char *strin = b.get();
        if(p1<=0){
            a->pushString("");
            return;
        }
        int len = wstrlen(strin);
        
        // this is how many chars we convert
        p1 = p1<len ? p1 : len;
        
        // we need to convert the entire string
        wchar_t *s = (wchar_t *)alloca((len+1)*sizeof(wchar_t));
        int rv = mbstowcs(s,strin,len+1);
        if(rv<0){
            a->pushNone();
            return;
        }
        
        // work out the start 
        int start = len-p1;
        
        // now convert back
        int l2 = wcstombs(NULL,s+start,0)+1;
        char *s2 = (char *)alloca(l2);
        wcstombs(s2,s+start,l2);
        a->pushString(s2);
    }
}

%wordargs splitlist li (coll n -- [list,list]) split list into two lists of [n,count-n] items
Creates two new lists, returned as elements of a list, consisting of the
first n items in the first list and the remainder in the second list.
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
Return the last item of a list or NONE if list is empty.
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

%word min ([a,b...]|a b -- minimum) Find minimum value in iterable or pair
This either a single iterable or a pair of values.
{
    StdComparator cmp(a);
    Value m;
    Value *i1 = a->popval(); // pop first item
    
    if(i1->t->flags & TF_ITERABLE){
        // if an iterable just run over those.
        Iterator<Value *> *iter = i1->t->makeIterator(i1);
        if(iter->isDone())
            m.clr();
        else {
            iter->first();
            m.copy(iter->current());
            if(!iter->isDone()){
                for(;!iter->isDone();iter->next()){
                    if(cmp.compare(iter->current(),&m)<0){
                        m.copy(iter->current());
                    }
                }
            }
        }
        delete iter;
    }else{
        // if it's not an iterable, we pop TWO items. Sadly comparators
        // are destructive. I could fix that and I should, but it's
        // really tedious to fix. This is just an extra copy.
        m.copy(a->popval());
        m.copy( (cmp.compare(i1,&m)<0) ? i1 : &m);
    }        
    a->pushval()->copy(&m);
}

%word max ([a,b...]|a b -- maximum) Find maximum value in iterable or pair
This either a single iterable or a pair of values.
{
    StdComparator cmp(a);
    Value m;
    Value *i1 = a->popval(); // pop first item
    
    if(i1->t->flags & TF_ITERABLE){
        // if an iterable just run over those.
        Iterator<Value *> *iter = i1->t->makeIterator(i1);
        if(iter->isDone())
            m.clr();
        else {
            iter->first();
            m.copy(iter->current());
            if(!iter->isDone()){
                for(;!iter->isDone();iter->next()){
                    if(cmp.compare(iter->current(),&m)>0){
                        m.copy(iter->current());
                    }
                }
            }
        }
        delete iter;
    }else{
        // if it's not an iterable, we pop TWO items. Sadly comparators
        // are destructive. I could fix that and I should, but it's
        // really tedious to fix. This is just an extra copy.
        m.copy(a->popval());
        int q = cmp.compare(i1,&m);
        m.copy( q>0 ? i1 : &m);
    }        
    a->pushval()->copy(&m);
}

%word maxf ( ([a,b...]| a b) func -- maximum) Find maximum value in iterable or pair using function
This uses the supplied function as a comparator to find the maximum item in an iterable or a pair. 
It takes a function and either a single iterable or a pair of values. There is no corresponding
minf function, since this can be constructed by negating the output of the function which
would be passed to maxf.
{
    Value func;
    Value m;
    
    func.copy(a->popval());
    FuncComparator cmp(a,&func);
    
    // see max and min for how we deal with pairs and iterables
    // differently
    Value *i1 = a->popval();
    
    if(i1->t->flags & TF_ITERABLE){
        Iterator<Value *> *iter = i1->t->makeIterator(i1);
        if(iter->isDone())
            m.clr();
        else {
            iter->first();
            m.copy(iter->current());
            if(!iter->isDone()){
                for(;!iter->isDone();iter->next()){
                    if(cmp.compare(iter->current(),&m)>0){
                        m.copy(iter->current());
                    }
                }
            }
        }
        delete iter;
    } else {
        Value v;
        v.copy(i1); // to avoid overwrite
        m.copy(a->popval());
        int q = cmp.compare(&v,&m);
        m.copy( q>0 ? &v : &m);
    }
    a->pushval()->copy(&m);
}






%word explode ([x,y,..] -- x,y,..) put all items onto stack
Puts all items from the list onto the stack, permitting the metaphor
[a,b,c] explode !c !b !a. Note the reversal of the order - a typical
usage is "reverse explode" (but this is slow).
{
    Value *iterable = a->popval();
    Iterator<Value *> *iter = iterable->t->makeIterator(iterable);
    
    for(iter->first();!iter->isDone();iter->next()){
        a->pushval()->copy(iter->current());
    }
    delete iter;
}    

%word fst (coll -- item/none) get first item
Return the first item of a list or NONE if list is empty.
{
    getByIndex(a->stack.peekptr(),0);
}

%word snd (coll -- item/none) get second item
Return the 2nd item of a list or NONE if not present.
{
    getByIndex(a->stack.peekptr(),1);
}
%word third (coll -- item/none) get second item
Return the 3rd item of a list or NONE if not present.
{
    getByIndex(a->stack.peekptr(),2);
}
%word fourth (coll -- item/none) get second item
Return the 4th item of a list or NONE if not present.
{
    getByIndex(a->stack.peekptr(),3);
}


%word get (key coll --) get an item from a list or hash
For a list, get the key'th item (starting from zero). For a hash, look up
the key and return the item. Lists will throw a ex$outofrange exception
if out of range, hashes will return NONE if item not found.
{
    Value *c = a->popval();
    Value *keyAndResult = a->stack.peekptr();
    Value v;
    c->t->getValue(c,keyAndResult,&v);
    keyAndResult->copy(&v); // copy into the key's slot
}
%word set (val key coll --) put an item into a list or hash
Store an item in a list of hash. Lists will be expanded to the required
size, with all unset items set to NONE. This can result in enormous
memory usage: lists are not stored sparsely.
{
    Value *c = a->popval();
    Value *k = a->popval();
    Value *v = a->popval();
    c->t->setValue(c,k,v);
}
%word len (list --) get length of list, hash or string
Get the number of items in a list, hash or string.
{
    Value *c = a->stack.peekptr();
    int ct = c->t->getCount(c);
    Types::tInteger->set(c,ct);
}

%word remove (key list -- item) remove an item by key or index, returning it.
Removes the nth item of a list, moving all subsequent items down.
In a hash, removes the item given by the key. In both cases the
removed item is returned.
{
    Value *c = a->popval();
    Value *keyAndResult = a->stack.peekptr();
    Value v;
    c->t->removeAndReturn(c,keyAndResult,&v);
    keyAndResult->copy(&v); // copy into the key's slot
}


%word shift (list -- item) remove and return the first item of the list
Removes and returns the first item of the list, throwing ex$outofrange
if there are no items.
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->pushval();
    
    v->copy(list->get(0));
    list->remove(0);
}

%word unshift (item list --) prepend an item to a list
Adds an item to the start of a list.
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->popval();
    list->insert(0)->copy(v);
}

%word pop (list -- item) pop an item from the end of the list
Removes and returns the last item of the list, throwing ex$outofrange
if there are no items.
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
Adds an item to the end of a list.
{
    ArrayList<Value> *list = Types::tList->get(a->popval());
    Value *v = a->popval();
    list->append()->copy(v);
}


%word map (iter func -- list) apply a function to an iterable, giving a list
For each item in the iterable (list, range  or hash) apply a function,
creating a new list containing the results. In the case of hashes, the
hash key is passed to the function.
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

%word reduce (accumulator iter func -- result) perform a (left) fold or reduce on an iterable.
Works on lists, ranges or hashes. For each item or hash key, calculate the
result of the binary function where the first operand is the accumulator
and the second is the item or key, and set the accumulator to this value.
Thus, "0 [1,2,3,4] (+) reduce" will sum the values.
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
For each item in an iterable, return a list of those items for which the
function returns nonzero. Accepts lists, ranges or hashes (in the latter case
the hash key is used).
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
For each item in an iterable, return two lists of those items for which the
function returns zero and those for which it returns nonzero. Accepts
lists, ranges or hashes (in the latter case the hash key is used).
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
Return true if an iterable (list, hash or integer range) contains
the item.
{
    Value *iterable = a->popval();
    Value *item = a->popval();
    
    bool b = iterable->t->getIndexOfContainedItem(iterable,item)>=0;
    
    a->pushInt(b?1:0);
}

%word index (item iterable -- int) return index of item in iterable, or none
Return the zero based index of an item in a list. Other iterables will return
none, non-iterable types will throw ex$noiter.
{
    Value *iterable = a->popval();
    Value *item = a->popval();
    
    int i = iterable->t->getIndexOfContainedItem(iterable,item);
    if(i<0)a->pushNone();
    else a->pushInt(i);
}

%word slice (iterable start len -- iterable) produce a slice of a string or list
Return a slice of a string or list, returning another string or or list,
given the start and length. Inappropriate types will throw ex$notcoll.
If length of the iterable  is greater than the length requested, then the
slice will go to the end. If the start<0 it is counted from the end.
Empty results will be returned if the requested slice does not intersect
the iterable.
{
    throw RUNT(EX_FAILED,"default 'slice' - import future or deprecated");
}

%word clone (in -- out) construct a shallow copy of a collection
Create a new collection (list or hash) of the same type, where
each item is a shallow copy. For example, copying a list of lists
creates a new list containing references to the lists in the original.
{
    Value *v = a->stack.peekptr();
    v->t->clone(v,v,false);
}

%word deepclone (in -- out) produce a deep copy of a value
Create a new collection (list or hash) of the same type, where
each item is a deep copy (i.e. a recursive copy). For example, copying
a list of lists creates a new list containing new copies of the original
lists.
{
    Value *v = a->stack.peekptr();
    v->t->clone(v,v,true);
    
}

%word sort (in --) sort a list in place using default comparator
Reorders the list using a standard comparison, which will fail if
the types of the items are not comparable.
{
    Value listv;
    // need copy because comparators use the stack
    listv.copy(a->popval());
    ArrayList<Value> *list = Types::tList->get(&listv);
    
    StdComparator cmp(a);
    list->sort(&cmp);
}

%word rsort (in --) reverse sort a list in place using default comparator
Reorders the list using a standard comparison, which will fail if
the types of the items are not comparable.
{
    Value listv;
    // need copy because comparators use the stack
    listv.copy(a->popval());
    ArrayList<Value> *list = Types::tList->get(&listv);
    
    RevStdComparator cmp(a);
    list->sort(&cmp);
}


%word fsort (in func --) sort a list in place using function comparator
Reorders the list using a binary function.
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
Returns true if the unary function provided returns nonzero for all the items
in the iterable. For hashes, this will use the key.
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
Returns true if the unary function provided returns nonzero for any item
in the iterable. For hashes, this will use the key.
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
For two lists, apply a binary function to pairs of items taken sequentially
from each list, creating a new list. Thus, "[1,2,3] [10,20,30] (+) zipWith"
will return "[11,22,33]". 
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
Reverse an iterable (list, hash, string, int range) creating
a new list of the results. Will work on hashes, but is pointless
since the keys are unordered.
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



%word intercalate (iter string -- string) concatenate strings of items with separator
For each element in a list, turn that element into a string. Concatenate
the strings together with the separator string supplied, and return the
result. Thus, '[1,2,3] "," intercalate' will return "1,2,3".
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
Create a new list, containing items in the iterable (list, hash etc.)
separated by the provided item.
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
