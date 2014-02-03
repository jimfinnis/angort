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

/// all Angort values are instances of these --- the type of a value is determined
/// by the t field, and the TypeData structure pointer.

struct Value {
    /// the type object for the value
    Type *t;
    
    union {
        float f;
        int i;
        char *s;
        void *v; //!< used in fixups
        struct BlockAllocHeader *block;
        const struct CodeBlock *cb;
        struct Closure *closure;
        
        struct Range *range;
        class ListObject *list;
        class GarbageCollected *gc;
        class IteratorObject *iter;
        uint32_t fixup;
    } v;
    
    
    Value(){
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
        strncpy(buf,t->toString(buf,len,this),len);
        return buf;
    }
    
    float toFloat(){
        return t->toFloat(this);
    }
    float toInt(){
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
        clr();
        
        t = src->t;
        v = src->v;
        incRef();
    }
    
    /// part of serialisation
    void load(class Serialiser *ser);
    /// part of serialisation
    void save(class Serialiser *ser);
    /// part of serialisation
    void setFixup(uint32_t fixup){
        t = Types::tFixup;
        v.fixup = fixup;
    }
    
    /// receive a visitor, with an optional name
    void receiveVisitor(ValueVisitor *vis,const char *name=NULL){
        if(vis->visit(name,this)){
            // recurse into the children if the visitor
            // doesn't abort
            t->visitRefChildren(this,vis);
        }
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
