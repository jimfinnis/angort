/**
 * @file list.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"
#include "cycle.h"

namespace angort {

ListObject::ListObject() : GarbageCollected(), list(32) {
    dprintf("LISTOBJECT create at %p\n",this);
}

ListObject::~ListObject(){
    dprintf("LISTOBJECT delete at %p\n",this);
    
    // just to make sure we don't do anything nasty once the
    // object has been DC'd, wipe the array.
    list.wipe();
    
}

void ListType::set(Value *v,ListObject *lo)const{
    v->clr();
    v->t = this;
    v->v.list = lo;
    incRef(v);
}

ArrayList<Value> *ListType::set(Value *v)const{
    ListObject *p = new ListObject();
    set(v,p);
    return &p->list;
}

ArrayList<Value> *ListType::get(Value *v)const{
    if(v->t != this)
        throw RUNT("ex$nolist","not a list");
    return &v->v.list->list;
}

// list iterator locking: on construction there's no lock;
// lock is created if required on first() and destroyed on end of loop or
// destructor.

class ListIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    int idx; //!< current index
    bool isKey;
    ListObject *list; //!< the range we're iterating over
    ReadLock *lock;
    
    /// copy the current value into the value
    inline void copyCurrent(){
        if(isKey)
            Types::tInteger->set(&v,idx);
        else
            v.copy(list->list.get(idx));
    }
    
public:
    /// create a list iterator for a list
    ListIterator(const ListObject *r,bool iskeyiterator){
        idx=0;
        lock = NULL; // starts with no lock
        isKey = iskeyiterator;
        list = (ListObject *)r;
        /// increment the list's reference count
        list->incRefCt();
    }
    
    /// on destruction, delete the iterator
    virtual ~ListIterator(){
        if(list->decRefCt()){
            delete list;
        }
        v.clr();
        if(lock){delete lock; lock=NULL;}
    }
    
    /// set the current value to the first item
    virtual void first(){
        idx=0;
        if(lock)delete lock;
        lock = new ReadLock(&list->list);
        if(idx<list->list.count())
            copyCurrent();
        else {
            v.clr();
            if(lock){delete lock; lock=NULL;}
        }
    }
    /// set the current value to the next item
    virtual void next(){
        idx++;
        if(idx<list->list.count())
            copyCurrent();
        else{
            v.clr();
            if(lock){delete lock; lock=NULL;}
        }
    }
    /// return true if we're out of bounds
    virtual bool isDone() const{
        return idx>=list->list.count();
    }
    
    /// return the current value
    virtual Value *current(){
        return &v;
    }
    virtual int index() const {
        return idx;
    }
};




Iterator<Value *> *ListObject::makeValueIterator()const{
    return new ListIterator(this,false);
}

Iterator<Value *> *ListObject::makeKeyIterator()const{
    return new ListIterator(this,true);
}


void ListType::setValue(Value *coll,Value *k,Value *v)const{
    ListObject *r = coll->v.list;
    WriteLock lock(&r->list);
    int i = k->toInt();
    r->list.set(i,v);
}

void ListType::getValue(Value *coll,Value *k,Value *result)const{
    ListObject *r = coll->v.list;
    ReadLock lock(&r->list);
    int i = k->toInt();
    result->copy(r->list.get(i));
}

int ListType::getCount(Value *coll)const{
    ListObject *r = coll->v.list;
    ReadLock lock(&r->list);
    return r->list.count();
}
void ListType::removeAndReturn(Value *coll,Value *k,Value *result)const{
    ListObject *r = coll->v.list;
    WriteLock lock(&r->list);
    int i = k->toInt();
    // will throw if out of range
    result->copy(r->list.get(i));
    r->list.remove(i);
}

/// deprecated version.
void ListType::slice_dep(Value *out,Value *coll,int start,int len)const{
    ArrayList<Value> *outlist = set(out);
    ArrayList<Value> *list = get(coll);
    
    WriteLock lock1(outlist);
    ReadLock lock2(list);
    
    int listlen = list->count();
    if(start<0)start=listlen+start;
    if(start<0)start=0;
    if(len<0)len=listlen;
    if(start<listlen){
        if(len>(listlen-start))
            len = listlen-start;
        for(int i=0;i<len;i++){
            Value *v = list->get(start+i);
            outlist->append()->copy(v);
        }
    }
}

/// good version
void ListType::slice(Value *out,Value *coll,int startin,int endin)const{
    ArrayList<Value> *outlist = set(out);
    ArrayList<Value> *list = get(coll);
    WriteLock lock1(outlist);
    ReadLock lock2(list);
    
    int start,end;
    int listlen = list->count();
    bool ok = getSliceEndpoints(&start,&end,listlen,startin,endin);
    
    if(ok){
        for(int i=start;i<end;i++){
            Value *v = list->get(i);
            outlist->append()->copy(v);
        }
    }
}

void ListType::clone(Value *out,const Value *in,bool deep)const{
    ListObject *p = new ListObject();
    
    WriteLock(&p->list);
    
    // cast away constness - makeIterator() can't be const
    // because it modifies refcounts
    ListIterator iter(in->v.list,false);
    for(iter.first();!iter.isDone();iter.next()){
        Value *v = p->list.append();
        if(deep)
            iter.current()->t->clone(v,iter.current(),true);
        else
            v->copy(iter.current());
    }
    
    set(out,p);
}

}
