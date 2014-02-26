#ifndef __TYPES_H
#define __TYPES_H

struct Value;
class Serialiser;

#include "iterator.h"
#include "gc.h"
#include "arraylist.h"

/// abstract type for something which can visit the tree of GarbageCollected
/// and BlockAllocType objects.

class ValueVisitor {
public:
    /// returns false if the recursion should halt - make sure it
    /// does so! If globname is not NULL, this is a global variable.
    virtual bool visit(const char *globname, Value *v) =0;
};




/// Each Value as a pointer to one of these, which exist as a set of
/// singletons describing each type's allocation behaviour etc.
/// We do this rather than have a huge VFT for each value.

class Type {
private:
    /// head of list of types
    static Type *head; 
    /// next pointer for type list
    Type *next;
public:
    /// a constant name
    const char *name;
    /// and ID for serialisation
    uint32_t id;
    
    /// true if the type is a reference type;
    /// that is, two values of this type can point to
    /// the same object.
    
    virtual bool isReference(){
        return false;
    }
    
    /// set the ID and name and add to the type list
    void add(const char *_name, const char *_id){
        name = _name;
        const unsigned char *n = (const unsigned char *)_id;
        id = n[0]+(n[1]<<8)+(n[2]<<16)+(n[3]<<24);
        
        next = head;
        head = this;
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
    
    /// convert to a string - this is the default, which does very little;
    /// USE THE RETURN VALUE because the buffer might be unused if no conversion
    /// was required.
    virtual const char * toString(char *outBuf,int len,const Value *v) const;
   
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
    
    /// recursively visit all the children of this value which
    /// are reference types (GCType or BlockAllocType.) Does nothing
    /// for primitive types.
    virtual void visitRefChildren(Value *v,ValueVisitor *visitor){
    }
    
    /// save a value, but not the data associated with a reference
    /// type; for those, we save the fixup.
    virtual void saveValue(Serialiser *ser, Value *v){
        throw SerialisationException(NULL)
              .set("cannot serialise type '%s'",name);
    }
    
    /// load a value, but not the data associated with a reference
    /// type; for those, we load the fixup.
    virtual void loadValue(Serialiser *ser, Value *v){
        throw SerialisationException(NULL)
              .set("cannot serialise type '%s'",name);
    }
    
    
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v){
        throw WTF;
    }
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser){
        throw WTF;
    }
    
    /// create the low-level iterator if possible
    virtual Iterator<Value *> *makeIterator(Value *v){
        throw RUNT("cannot iterate a non-iterable type");
    }
    
    /// find a type by ID
    static Type *findByID(uint32_t id){
        for(Type *p = head;p;p=p->next)
            if(p->id == id)
                return p;
        return NULL;
    }
    
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
    char *allocate(Value *v,int len);
    /// return a pointer to the allocated data (AFTER the header)
    const char *getData(const Value *v) const;
    
    /// standard fixup resolution
    virtual void saveValue(Serialiser *ser, Value *v);
    /// standard fixup resolution
    virtual void loadValue(Serialiser *ser, Value *v);
};

/// this is for GC types which contain a GarbageCollected item
class GCType : public Type {
public:
    virtual bool isReference(){
        return true;
    }
    
    
    /// standard fixup resolution
    virtual void saveValue(Serialiser *ser, Value *v);
    /// standard fixup resolution
    virtual void loadValue(Serialiser *ser, Value *v);
    
    /// serialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void saveDataBlock(Serialiser *ser,const void *v){
        throw WTF;
    }
    /// deserialise the data for a reference type - that is, the data
    /// which is held external to the Value union.
    virtual void *loadDataBlock(Serialiser *ser){
        throw WTF;
    }
    /// increment the reference count
    virtual void incRef(Value *v);
    
    /// decrement the reference count and delete if zero
    virtual void decRef(Value *v);
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


/// this is effectively a namespace for the type data
struct Types {
    static void createTypes();
    
    /// the null type object
    static Type *tNone;
    /// the type object for deleted hash keys
    static Type *tDeleted;
    /// not a real type; d.fixup gives the ID of a datum in the fixup table;
    /// used in serialisation
    static Type *tFixup;
    
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
    
    /// v.gc is some unspecified garbage-collected type
    static GCType *tGC;
};

#endif /* __TYPES_H */
