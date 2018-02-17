/**
 * @file list.h
 * @brief  Brief description of file.
 *
 */

#ifndef __ANGORTLIST_H
#define __ANGORTLIST_H

namespace angort {

struct ListObject : public GarbageCollected {
    ArrayList<Value> list;
    virtual Iterator<class Value *> *makeValueIterator()const;
    virtual Iterator<class Value *> *makeKeyIterator()const;
    virtual void wipeContents();
    
    ListObject();
    ~ListObject();
};

class ListType : public GCType {
public:
    ListType(){
        add("list","LIST");
        flags |= TF_ITERABLE;
    }
    
    /// get this value's arraylist, throwing if it's not a list
    ArrayList<Value> *get(Value *v)const;
    
    /// create a new list
    ArrayList<Value> *set(Value *v)const;
    
    /// set a value to an existing list
    void set(Value *v,ListObject *lo)const;
    
    virtual void setValue(Value *coll,Value *k,Value *v)const;
    virtual void getValue(Value *coll,Value *k,Value *result)const;
    virtual int getCount(Value *coll)const;
    virtual void removeAndReturn(Value *coll,Value *k,Value *result)const;
    virtual void slice(Value *out,Value *coll,int start,int len)const;
    virtual void slice_dep(Value *out,Value *coll,int start,int len)const;
    
    virtual void clone(Value *out,const Value *in,bool deep=false)const;
    virtual class Lockable *getLockable(Value *v) const { return get(v); }
    
};

}
#endif /* __LIST_H */
