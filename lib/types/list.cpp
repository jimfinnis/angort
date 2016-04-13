/**
 * @file list.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"
#include "cycle.h"

namespace angort {

ListObject::ListObject() : GarbageCollected(), list(32) {
}

ListObject::~ListObject(){
}

void ListType::set(Value *v,ListObject *lo){
    v->clr();
    v->t = this;
    v->v.list = lo;
    incRef(v);
}

ArrayList<Value> *ListType::set(Value *v){
    ListObject *p = new ListObject();
    set(v,p);
    return &p->list;
}

ArrayList<Value> *ListType::get(Value *v){
    if(v->t != this)
        throw RUNT("ex$nolist","not a list");
    return &v->v.list->list;
}

class ListIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    int idx; //!< current index
    bool isKey;
    ListObject *list; //!< the range we're iterating over
    
    /// copy the current value into the value
    inline void copyCurrent(){
        if(isKey)
            Types::tInteger->set(&v,idx);
        else
            v.copy(list->list.get(idx));
    }
    
public:
    /// create a list iterator for a list
    ListIterator(ListObject *r,bool iskeyiterator){
        idx=0;
        isKey = iskeyiterator;
        list = r;
        /// increment the list's reference count
        list->incRefCt();
    }
    
    /// on destruction, delete the iterator
    virtual ~ListIterator(){
        if(list->decRefCt()){
            delete list;
        }
        v.clr();
    }
    
    /// set the current value to the first item
    virtual void first(){
        idx=0;
        if(idx<list->list.count())
            copyCurrent();
        else
            v.clr();
    }
    /// set the current value to the next item
    virtual void next(){
        idx++;
        if(idx<list->list.count())
            copyCurrent();
        else{
            v.clr();
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
};




Iterator<Value *> *ListObject::makeValueIterator(){
    return new ListIterator(this,false);
}

Iterator<Value *> *ListObject::makeKeyIterator(){
    return new ListIterator(this,true);
}


void ListType::setValue(Value *coll,Value *k,Value *v){
    ListObject *r = coll->v.list;
    int i = k->toInt();
    r->list.set(i,v);
}

void ListType::getValue(Value *coll,Value *k,Value *result){
    ListObject *r = coll->v.list;
    int i = k->toInt();
    result->copy(r->list.get(i));
}

int ListType::getCount(Value *coll){
    ListObject *r = coll->v.list;
    return r->list.count();
}
void ListType::removeAndReturn(Value *coll,Value *k,Value *result){
    ListObject *r = coll->v.list;
    int i = k->toInt();
    // will throw if out of range
    result->copy(r->list.get(i));
    r->list.remove(i);
}

void ListType::slice(Value *out,Value *coll,int start,int len){
    ArrayList<Value> *outlist = set(out);
    ArrayList<Value> *list = get(coll);
    
    int listlen = list->count();
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
void ListType::clone(Value *out,const Value *in,bool deep){
    ListObject *p = new ListObject();
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
