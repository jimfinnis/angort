/**
 * @file list.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"
#include "cycle.h"

ListObject::ListObject() : GarbageCollected(), list(32) {
    CycleDetector::getInstance()->add(this);
}

ListObject::~ListObject(){
    CycleDetector::getInstance()->remove(this);
}

ArrayList<Value> *ListType::set(Value *v){
    v->clr();
    v->t = this;
    
    ListObject *p = new ListObject();
    v->v.list = p;
    
    incRef(v);
    return &p->list;
}

ArrayList<Value> *ListType::get(Value *v){
    if(v->t != this)
        throw RUNT("not a list");
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


void ListType::visitRefChildren(Value *v,ValueVisitor *visitor){
    ListObject *r = v->v.list;
    
    for(int i=0;i<r->list.count();i++){
        r->list.get(i)->receiveVisitor(visitor);
    }
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

