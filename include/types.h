#ifndef __ANGORTTYPES_H
#define __ANGORTTYPES_H

#include "iterator.h"
#include "gc.h"
#include "arraylist.h"
#include "intkeyedhash.h"

namespace angort {

struct Value;

/// this is a function for a binary operator. These are stored in a
/// hash owned by the LHS types, indexed by the ID of the RHS types
/// combined somehow with the binop type.

typedef void (*BinopFunction)(Angort *a,Value *lhs, Value *rhs);

#define TF_ITERABLE 1
#define TF_NUMBER 2

/// Each Value has a pointer to one of these, which exist as a set of
/// singletons describing each type's allocation behaviour etc.

class Type {
    friend class Types;
    
    static Type *head; //!< head of list of types
    Type *next; //!< linking field of type list
    
    /// binary operators for which this is a left-hand side, keyed
    /// by right-hand side binopID and opcode.
    IntKeyedHash<BinopFunction> binops;
    
public:
    
    Type(){
        flags=0;
    }
    
    /// TF_ flags giving properties (iterable, etc.)
    uint32_t flags;
    
    /// a constant name
    const char *name;
    /// a 32 bit unique ID (used in serialisation, etc.)
    uint32_t id;
    /// an internal ID used for binop hashing. We can't use the above,
    /// because it's a full 32 bits wide; we need spare bits in this
    /// one for the binop ID. Memory wasteful, but we don't have a vast
    /// number of type objects.
    uint32_t binopID;
    
    /// a symbol for the above string - this is what angort stacks
    /// when the "type" word is called on a value.
    int nameSymb;
    
    /// scan global list for a type by ID
    static Type *getByID(const char *_id);
    
    /// convert to a UTF-8 string - if memory was allocated,
    /// the boolean is set.
    virtual const char *toString(bool *allocated, const Value *v) const;
    
    /// true if the type is a reference type;
    /// that is, two values of this type can point to
    /// the same object.
    
    virtual bool isReference() const{
        return false;
    }
    
    /// true if this is callable; in which case a global
    /// of this name will be run rather than stacked (unless ? is used)
    virtual bool isCallable() const{
        return false;
    }
    
    /// return the GC object only if this is a GC type
    virtual class GarbageCollected *getGC(Value *v) const{
        return NULL;
    }
          
    
    /// set name of type, add to global type list
    /// ID is a 4-byte identifier.
    void add(const char *_name,const char *_id);
    
    /// register a binary operation for this type as the LHS
    /// and another as the RHS. 
    void registerBinop(Type *rhs, int opcode, BinopFunction f);
    
    /// get the binary operation for this type as the LHS and
    /// another as the LHS, or NULL.
    inline BinopFunction *getBinop(const Type *rhs,int opcode){
        uint32_t key = (rhs->binopID << 16) + opcode;
        return binops.ffind(key);
    }
    
    /// look up and perform a registered binary operation, returning
    /// true if one was found.
    static bool binop(Angort *a,int opcode,Value *lhs,Value *rhs);
    
    /// reset the type list, does not delete anything because
    /// the type objects are static
    static void clearList(){
        head = NULL;
    }
    
    static Type *getByName(const char *n){
        for(Type *p=head;p;p=p->next)
            if(!strcmp(n,p->name))
                return p;
        return NULL;
    }
    
    static void dumpTypes(){
        for(Type *p=head;p;p=p->next)
            printf("%s\n",p->name);
    }
    
    /// dereference a reference value, returning the value to which
    /// it returns or NULL if it's not a reference.
    virtual Value *deref(Value *v) const {
        return NULL; // not a reference
    }
    
    /// store a value into a reference value of this type
    virtual void store(Value *ref,Value *v) const{
        throw RUNT(EX_NOTREF,"trying to store a value in a non-reference value");
    }
    
    /// convert a value to another value of my type, generally used
    /// in type coercion of arguments. Throws a BadConversionException
    /// on failure.
    virtual void toSelf(Value *out,const Value *v) const;
    
    /// convert to a float - exception by default
    virtual float toFloat(const Value *v) const;
    /// convert to an int - exception by default
    virtual int toInt(const Value *v) const;
    /// convert to a long - exception by default
    virtual long toLong(const Value *v) const;
    /// convert to a double - exception by default
    virtual double toDouble(const Value *v) const;
    
    /// create a low-level iterator and then wrap it in an iterator value
    void createIterator(Value *dest,Value *src)const;
    
    virtual uint32_t getHash(Value *v)const{
        throw RUNT(EX_NOHASH,"type is not hashable");
        return 0;
    }
    
    virtual bool equalForHashTable(Value *a,Value *b)const{
        throw RUNT(EX_NOHASH,"type is not hashable");
        return false;
    }
        
    
    
    
    /// increment the reference count, default does nothing
    virtual void incRef(Value *v)const{
    }
    
    /// decrement the reference count and delete if zero, default does nothing
    virtual void decRef(Value *v)const{
    }
    
    /// the "default" iterator is a value iterator for lists etc,
    /// and a key iterator for hashes.
    virtual Iterator<Value *> *makeIterator(Value *v)const{
        return makeValueIterator(v);
    }
    
    /// create the low-level iterator if possible. GCType does this by calling
    /// makeValueIterator on the underlying GarbageCollected object in the value.
    virtual Iterator<Value *> *makeValueIterator(Value *v)const{
        throw RUNT(EX_NOITER,"cannot iterate a non-iterable value");
    }
    
    /// create the low-level key iterator if possible. GCType does this by calling
    /// makeValueIterator on the underlying GarbageCollected object in the value.
    virtual Iterator<Value *> *makeKeyIterator(Value *v)const{
        throw RUNT(EX_NOITER,"cannot iterate a non-iterable value");
    }
    
    /// return the index of item is in the collection (uses the same
    /// equality test as hash keys).If not present, returns -1.
    virtual int getIndexOfContainedItem(Value *v,Value *item)const;
        
    
    /// set a value in a collection, if this type is one
    virtual void setValue(Value *coll,Value *k,Value *v)const{
        throw RUNT(EX_NOTCOLL,"cannot set value of item inside a non-collection");
    }
    /// get a value from a collection, if this type is one
    virtual void getValue(Value *coll,Value *k,Value *result)const{
        throw RUNT(EX_NOTCOLL,"cannot get value of item inside a non-collection");
    }
    /// get number of items in a collection if this is one
    virtual int getCount(Value *coll)const{
        throw RUNT(EX_NOTCOLL,"cannot get count of non-collection");
    }
    virtual void removeAndReturn(Value *coll,Value *k,Value *result)const{
        throw RUNT(EX_NOTCOLL,"cannot remove from non-collection");
    }
    
    virtual void slice(Value *out,Value *coll,int start,int len)const{
        throw RUNT(EX_NOTCOLL,"cannot get slice of non-collection");
    }
    
    /// generate a shallow or deep copy of the object:
    /// default action is to just copy the value; collections
    /// need to do more. Note that in and out may point to
    /// the same Value.
    virtual void clone(Value *out,const Value *in,bool deep=false) const;
    
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
    
    virtual bool isReference()const{
        return true;
    }
    /// increment the reference count
    virtual void incRef(Value *v)const;
    
    /// decrement the reference count and delete if zero
    virtual void decRef(Value *v)const;
    
    
    /// allocate a block of memory plus 16 bits for refcount, 
    /// setting the refcount to 1, setting v.s to the start of the whole block,
    /// returning a pointer to just after the header.
    /// Also clears the value type and sets the new type.
    char *allocate(Value *v,int len,const Type *t)const;
    /// return a pointer to the allocated data (AFTER the header)
    const char *getData(const Value *v) const;
};

/// this is for GC types which contain a GarbageCollected item
class GCType : public Type {
public:
    virtual bool isReference()const{
        return true;
    }
    
    
    /// increment the reference count
    virtual void incRef(Value *v)const;
    
    /// decrement the reference count and delete if zero
    virtual void decRef(Value *v)const;

    virtual class GarbageCollected *getGC(Value *v)const;
    
    /// make an iterator by calling the gc's method
    virtual Iterator<class Value *> *makeValueIterator(Value *v)const;
    /// make an iterator by calling the gc's method
    virtual Iterator<class Value *> *makeKeyIterator(Value *v)const;
    
};

}

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
#include "types/long.h"
#include "types/double.h"


namespace angort {

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
    /// v.l is a long
    static LongType *tLong;
    /// v.df is a double
    static DoubleType *tDouble;
    
    
    /// v.gc is some unspecified garbage-collected type
    static GCType *tGC;
};

}
#endif /* __TYPES_H */
