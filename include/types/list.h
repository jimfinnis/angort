/**
 * @file list.h
 * @brief  Brief description of file.
 *
 */

#ifndef __LIST_H
#define __LIST_H

#include "angort.h"

struct ListObject : public GarbageCollected {
    ArrayList<Value> list;
    virtual Iterator<class Value *> *makeValueIterator();
    virtual Iterator<class Value *> *makeKeyIterator();
    
    ListObject();
    ~ListObject();
};

class ListType : public GCType {
public:
    ListType(){
        add("list","LIST");
    }
    
    /// get this value's arraylist, throwing if it's not a list
    ArrayList<Value> *get(Value *v);
    
    /// create a new list
    ArrayList<Value> *set(Value *v);
    
    /// set a value to an existing list
    void set(Value *v,ListObject *lo);
    
    virtual void setValue(Value *coll,Value *k,Value *v);
    virtual void getValue(Value *coll,Value *k,Value *result);
    virtual int getCount(Value *coll);
    virtual void removeAndReturn(Value *coll,Value *k,Value *result);
    virtual void slice(Value *out,Value *coll,int start,int len);
    virtual void clone(Value *out,const Value *in);
};


#endif /* __LIST_H */
