#ifndef __TYPES_H
#define __TYPES_H

struct Value;

#include "iterator.h"
#include "gc.h"
#include "arraylist.h"


/// Each Value has a pointer to one of these, which exist as a set of
/// singletons describing each type's allocation behaviour etc.

class Type {
    friend class Types;
    
    static Type *head; //!< head of list of types
    Type *next; //!< linking field of type list
public:
    /// a constant name
    const char *name;
    /// a symbol for the above string - this is what angort stacks
    /// when the "type" word is called on a value.
    int nameSymb;
    
    /// convert to a UTF-8 string - if memory was allocated,
    /// the boolean is set.
    virtual const char *toString(bool *allocated, const Value *v) const;
    
    /// true if the type is a reference type;
    /// that is, two values of this type can point to
    /// the same object.
    
    virtual bool isReference(){
        return false;
    }
    
    /// true if this is callable; in which case a global
    /// of this name will be run rather than stacked (unless ? is used)
    virtual bool isCallable(){
        return false;
    }
    
    /// return the GC object only if this is a GC type
    virtual class GarbageCollected *getGC(Value *v){
        return NULL;
    }
          
    
    /// set name of type, add to global type list
    void add(const char *_name);
    
    /// reset the type list, does not delete anything because
    /// the type objects are static
    static void clearList(){
        head = NULL;
    }
    
    /// dereference a reference value, returning the value to which
    /// it returns or NULL if it's not a reference.
    virtual Value *deref(Value *v) const {
        return NULL; // not a reference
    }
    
    /// store a value into a reference value of this type
    virtual void store(Value *ref,Value *v) const{
        throw RUNT("trying to store a value in a non-reference value");
    }
    
    /// convert to a float - exception by default
    virtual float toFloat(const Value *v) const;
    /// convert to an int - exception by default
    virtual int toInt(const Value *v) const;
    
    /// create a low-level iterator and then wrap it in an iterator value
    void createIterator(Value *dest,Value *src);
    
    virtual uint32_t getHash(Value *v){
        throw RUNT("type is not hashable");
        return 0;
    }
    
    virtual bool equalForHashTable(Value *a,Value *b){
        throw RUNT("type is not hashable");
        return false;
    }
        
    
    
    
    /// increment the reference count, default does nothing
    virtual void incRef(Value *v){
    }
    
    /// decrement the reference count and delete if zero, default does nothing
    virtual void decRef(Value *v){
    }
    
    /// the "default" iterator is a value iterator for lists etc,
    /// and a key iterator for hashes.
    virtual Iterator<Value *> *makeIterator(Value *v){
        return makeValueIterator(v);
    }
    
    /// create the low-level iterator if possible. GCType does this by calling
    /// makeValueIterator on the underlying GarbageCollected object in the value.
    virtual Iterator<Value *> *makeValueIterator(Value *v){
        throw RUNT("cannot iterate a non-iterable value");
    }
    
    /// create the low-level key iterator if possible. GCType does this by calling
    /// makeValueIterator on the underlying GarbageCollected object in the value.
    virtual Iterator<Value *> *makeKeyIterator(Value *v){
        throw RUNT("cannot iterate a non-iterable value");
    }
    
    /// return whether the item is in the collection (uses the same
    /// equality test as hash keys)
    virtual bool isIn(Value *v,Value *item);
        
    
    /// set a value in a collection, if this type is one
    virtual void setValue(Value *coll,Value *k,Value *v){
        throw RUNT("cannot set value of item inside a non-collection");
    }
    /// get a value from a collection, if this type is one
    virtual void getValue(Value *coll,Value *k,Value *result){
        throw RUNT("cannot get value of item inside a non-collection");
    }
    /// get number of items in a collection if this is one
    virtual int getCount(Value *coll){
        throw RUNT("cannot get count of non-collection");
    }
    virtual void removeAndReturn(Value *coll,Value *k,Value *result){
        throw RUNT("cannot remove from non-collection");
    }
    
    virtual void slice(Value *out,Value *coll,int start,int len){
        throw RUNT("cannot get slice of non-collection");
    }
    
    /// generate a shallow or deep copy of the object:
    /// default action is to just copy the value; collections
    /// need to do more. Note that in and out may point to
    /// the same Value.
    virtual void clone(Value *out,const Value *in,bool deep=false);
protected:
    
};

/// this is the start of a BlockAllocType piece of data.
struct BlockAllocHeader {
    unsigned short refct;
    // actual data follows
};


/// this is for values where the s field is a pointer to a block of
/// memory which consists of a 16bit refcount followed by the data itself

class BlockAllocType : public Type {
public:
    
    virtual bool isReference(){
        return true;
    }
    /// increment the reference count
    virtual void incRef(Value *v);
    
    /// decrement the reference count and delete if zero
    virtual void decRef(Value *v);
    
    
    /// allocate a block of memory plus 16 bits for refcount, 
    /// setting the refcount to 1, setting v.s to the start of the whole block,
    /// returning a pointer to just after the header.
    /// Also clears the value type and sets the new type.
    char *allocate(Value *v,int len,Type *t);
    /// return a pointer to the allocated data (AFTER the header)
    const char *getData(const Value *v) const;
};

/// this is for GC types which contain a GarbageCollected item
class GCType : public Type {
public:
    virtual bool isReference(){
        return true;
    }
    
    
    /// increment the reference count
    virtual void incRef(Value *v);
    
    /// decrement the reference count and delete if zero
    virtual void decRef(Value *v);

    virtual class GarbageCollected *getGC(Value *v);
    
    /// make an iterator by calling the gc's method
    virtual Iterator<class Value *> *makeValueIterator(Value *v);
    /// make an iterator by calling the gc's method
    virtual Iterator<class Value *> *makeKeyIterator(Value *v);
        
};



/*
 * Add new types down here and in types.cpp
 */


#include "types/integer.h"
#include "types/float.h"
#include "types/code.h"
#include "types/closure.h"
#include "types/string.h"
#include "types/range.h"
#include "types/list.h"
#include "types/iter.h"
#include "types/hashtype.h"
#include "types/symbol.h"
#include "types/none.h"
#include "types/native.h"
#include "types/plugobj.h"


/// this is effectively a namespace for the type data
struct Types {
    static void createTypes();
    
    /// the null type
    static NoneType *tNone;
    /// the type object for deleted hash keys
    static Type *tDeleted;
    
    /// the d.i value gives the integer value
    static IntegerType *tInteger;
    /// the d.f value gives the float value
    static FloatType *tFloat;
    /// v.s is an offset into the constant data area giving
    /// a pointer to an instruction, i.e. a user function
    /// the value is a string, to which the d.s pointer points.
    static StringType *tString;
    /// v.cb is a code block
    static CodeType *tCode;
    /// v.closure is a closure, which is garbage collected
    static ClosureType *tClosure;
    /// v.irange is a range object
    static RangeType<int> *tIRange;
    /// v.frange is a range object
    static RangeType<float> *tFRange;
    /// v.iter is an iterator object
    static IteratorType *tIter;
    /// v.list is a list object
    static ListType *tList;
    /// v.lhash is a list object
    static HashType *tHash;
    /// v.i is a symbol ID
    static SymbolType *tSymbol;
    /// v.native is a native function
    static NativeType *tNative;
    /// v.property is a property
    static PropType *tProp;
    
    /// v.pluginfunc is a plugin function in a .angso
    static PluginFuncType *tPluginFunc;
    /// v.plobj is a wrapper around PluginObject
    static PluginObjectType *tPluginObject;
    
    /// v.gc is some unspecified garbage-collected type
    static GCType *tGC;
};

#endif /* __TYPES_H */
