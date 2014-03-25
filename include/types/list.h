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
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser);
    
    virtual void visitRefChildren(Value *v,ValueVisitor *visitor);
    
    virtual Iterator<Value *> *makeIterator(Value *v);
    
    virtual void setValue(Value *coll,Value *k,Value *v);
    virtual void getValue(Value *coll,Value *k,Value *result);
    virtual int getCount(Value *coll);
    virtual void removeAndReturn(Value *coll,Value *k,Value *result);
};


#endif /* __LIST_H */
