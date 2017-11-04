/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __ANGORTVALUE_H
#define __ANGORTVALUE_H

#include "types.h"

namespace angort {

/// all Angort values are instances of these --- the type of a value is
/// determined by the t field, and the TypeData structure pointer. 
/// Why not just have a hierarchy of Value types, with a VFT defining
/// the operations rather than a Type objects, and no union? It's because
/// very many data structures in Angort require the ability to store
/// many different types of values in an array, and also because they
/// need to be able to change the types of values in this array.

struct Value {
    /// the type object for the value
    const Type *t;
    
    union {
        float f;
        int i;
        long l;
        double df;
        char *s;
        struct BlockAllocHeader *block;
        const struct CodeBlock *cb;
        
        struct Range<int> *irange;
        struct Range<float> *frange;
        class ListObject *list;
        class HashObject *hash;
        class GarbageCollected *gc;
        class IteratorObject *iter;
        NativeFunc native;
        struct Property *property;
        class Closure *closure;
        void *v;
    } v;
    
    
    Value(){
        init();
    }
    
    /// do not use this on a variable which may already contain a value;
    /// use clr instead.
    void init(){
        t = Types::tNone;
    }
        
    
    ~Value(){
        clr();
    }
    
    /// decrement reference count and set type to NONE
    void clr(){
        if(t&&t!=Types::tNone){
            decRef();
            t=Types::tNone;
        }
    }
    
    /// decrement the reference count and deallocate if zero and is a type with extra stuff
    void decRef(){
        t->decRef(this);
    }
    
    /// increment the reference count
    void incRef(){
        t->incRef(this);
    }
    
    const Type *getType() const{
        return t;
    }
    
    const StringBuffer toString() const {
        return StringBuffer(this);
    }
    
    float toFloat() const {
        return t->toFloat(this);
    }
    int toInt() const {
        return t->toInt(this);
    }
    
    /// convert to a boolean:
    /// none is always false
    /// integers are false if zero
    /// anything else is true.
    bool toBool() const {
        if(isNone())
            return false;
        else if(t == Types::tInteger)
            return toInt()!=0;
        else
            return true;
    }
    long toLong() const {
        return t->toLong(this);
    }
    double toDouble() const {
        return t->toDouble(this);
    }
    
    void *getRaw(){
        return v.s;
    }
    
    void setNone(){
        clr();
    }
    
    bool isNone() const {
        return t==Types::tNone;
    }
    
    /// increment or decrement a value (via its type)
    void increment(int step){
        t->increment(this,step);
    }
    
    /// copy of a reference type (that's a type which refers
    /// to a bit of memory somewhere else, like a STRING or CLOSURE) will just copy the reference
    /// and increment the refct.
    /// Compare clone() which makes a shallow copy of the data itself.
    void copy(const Value *src){
        
        // don't do anything if src==this
        if(src==this)
            return;
        
        // we do this weird stuff to avoid situation where we try to copy 
        // a value out of a GC into a value which holds the GC itself with
        // one reference. Here, the GC would actually get deleted by clr()
        // before the copy. To avoid this, we artificially hike the refct
        // before the copy, and drop it afterwards in a slightly odd operation.
        // It may well slow things down, a little.
        
        Value old;
        old.t = t;
        old.v = v;
        old.incRef();
        
        
        clr();
        
        t = src->t;
        v = src->v;
        incRef();
        // "old" will decref on destruction
    }
    
    /// copy of a type but this time a true copy, rather than a copy
    /// of the reference. Compare copy().
    void clone(const Value *src){
        t->clone(this,src);
    }
    
    /// get a hash integer for this value
    uint32_t getHash(){
        return t->getHash(this);
    }
    
    /// are these two equal (considered as keys for hashes?)
    bool equalForHashTable(Value *other){
        if(t != other->t)
            return false;
        return t->equalForHashTable(this,other);
    }
    
    /// debugging method - dump a value to stdout, using the string
    /// from toString().
    void dump(const char *prefix){
        printf("%s: %s\n",prefix,toString().get());
    }
    
//private:    
    
    /// make sure the string gets copied on assignment
    Value &operator=(const Value &src){
        copy(&src);
        return *this;
    }
    /// make sure the string gets copied on copy-create
    Value(const Value &src){
        copy(&src);
    }
};

}

#endif /* __VALUE_H */
