/**
 * @file list.cpp
 * @brief  Brief description of file.
 *
 */

#include "angort.h"
#include "file.h"
#include "ser.h"

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

struct ListIterator : public Iterator<Value *>{
    Value v; //!< the current value, as an actual value
    int idx; //!< current index
    
    ListObject *list; //!< the range we're iterating over
public:
    /// create a list iterator for a list
    ListIterator(ListObject *r){
        idx=0;
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
            v.copy(list->list.get(idx));
        else
            v.clr();
    }
    /// set the current value to the next item
    virtual void next(){
        idx++;
        if(idx<list->list.count())
            v.copy(list->list.get(idx));
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

Iterator<Value *> *ListType::makeIterator(Value *v){
    return new ListIterator(v->v.list);
}

void ListType::saveDataBlock(Serialiser *ser,const void *v){
    ListObject *r = (ListObject *)v;
    ser->file->writeInt(r->list.count());
    for(int i=0;i<r->list.count();i++){
        r->list.get(i)->save(ser);
    }
}
void *ListType::loadDataBlock(Serialiser *ser){
    ListObject *r = new ListObject();
    
    int count = ser->file->readInt();
    for(int i=0;i<count;i++){
        Value *v = r->list.append();
        v->load(ser);
    }
    
    return (void *)r;
}


void ListType::visitRefChildren(Value *v,ValueVisitor *visitor){
    ListObject *r = v->v.list;
    
    for(int i=0;i<r->list.count();i++){
        r->list.get(i)->receiveVisitor(visitor);
    }
}
