/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __VALUE_H
#define __VALUE_H

#include "types.h"
#include "plugins.h"

/// all Angort values are instances of these --- the type of a value is determined
/// by the t field, and the TypeData structure pointer.

struct Value {
    /// the type object for the value
    Type *t;
    
    union {
        float f;
        int i;
        char *s;
        struct BlockAllocHeader *block;
        const struct CodeBlock *cb;
        struct Closure *closure;
        
        struct Range<int> *irange;
        struct Range<float> *frange;
        class ListObject *list;
        class HashObject *hash;
        class GarbageCollected *gc;
        class IteratorObject *iter;
        PluginFunc *native;
        class PluginObjectWrapper *plobj;
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
        if(t!=Types::tNone){
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
    
    Type *getType() const{
        return t;
    }
    
    const char * toString(char *buf,int len) const {
        return t->toString(buf,len,this);
    }
    
    float toFloat(){
        return t->toFloat(this);
    }
    int toInt(){
        return t->toInt(this);
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
    
    /// copy of a reference type (that's a type which refers
    /// to a bit of memory somewhere else, like a STRING or CLOSURE) will just copy the reference
    /// and increment the refct
    void copy(const Value *src){
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
        char buf[256];
        const char *s;
        s = toString(buf,256);
        printf("%s: %s\n",prefix,buf);
    }
    
private:    
    
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


#endif /* __VALUE_H */
