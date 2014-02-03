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
    
    ListObject() : GarbageCollected(), list(32){
    }
};

class ListType : public GCType {
public:
    ListType(){
        add("list","LIST");
    }
    
    /// get this value's arraylist, throwing if it's not a list
    ArrayList<Value> *get(Value *v);
    
    /// create a new list
    void set(Value *v);
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v);
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser);
    
    virtual void visitRefChildren(Value *v,ValueVisitor *visitor);
    
protected:
    virtual Iterator<Value *> *makeValueIterator(Value *v);
};


#endif /* __LIST_H */
