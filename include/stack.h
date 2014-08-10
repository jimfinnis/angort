/**
 * \file 
 *
 * 
 * \author $Author$
 * \date $Date$
 */


#ifndef __STACK_H
#define __STACK_H

#include "exception.h"

namespace angort {

/// the root stack exception
class StackException : public Exception {
public:
    StackException(const char *e) : Exception(e) {}
};

/// not enough items on the stack to pop()
class StackUnderflowException : public StackException {
public:
    StackUnderflowException() : StackException("stack underflow") {}
};

/// too many items on the stack to push()
class StackOverflowException : public StackException {
public:
    StackOverflowException() : StackException("stack overflow") {}
};



/// a class to encapsulate a simple stack of N items of type T, 
/// where items are copied into the stack - it is not a stack
/// of pointers.

template <class T,int N> class Stack {
public:
    
    Stack(){
        ct=0;
    }
    
    /// get an item from the top of the stack, discarding n items first.
    T pop(int n=0) {
        if(ct<=n)
            throw StackUnderflowException();
        ct -= n+1;
        return stack[ct];
    }
    
    /// get the nth item from the top of the stack
    T peek(int n=0) {
        if(ct<=n)
            throw StackUnderflowException();
        return stack[ct-(n+1)];
    }
    
    /// get a pointer to the nth item from the top of the stack
    T *peekptr(int n=0) {
        if(ct<=n)
            throw StackUnderflowException();
        return stack+(ct-(n+1));
    }
    
    /// get a pointer to an item from the top of the stack, discarding n items first,
    /// return NULL if there are not enough items instead of throwing an exception.
    T *popptrnoex(int n=0) {
        if(ct<=n)
            return NULL;
        ct -= n+1;
        return stack+ct;
    }
    
    /// get a pointer to an item from the top of the stack, discarding n items first
    T *popptr(int n=0) {
        if(ct<=n)
            throw StackUnderflowException();
        ct -= n+1;
        return stack+ct;
    }
    
    /// push an item onto the stack, returning the new item slot
    /// to be written into.
    T* pushptr() {
        if(ct==N)
            throw StackOverflowException();
        return stack+(ct++);
    }
    
    /// push an item onto the stack, passing the object in - consider
    /// using pushptr(), it might be quicker.
    void push(T o){
        if(ct==N)
            throw StackOverflowException();
        stack[ct++]=o;
    }
    
    /// swap the top two items on the stack
    void swap(){
        if(ct<2)
            throw StackUnderflowException();
        T x;
        x=stack[ct-1];
        stack[ct-1]=stack[ct-2];
        stack[ct-2]=x;
    }
    
    /// is the stack empty?
    bool isempty(){
        return ct==0;
    }
    
    /// empty the stack
    void clear() {
        ct=0;
    }
    
    /// left public for debugging handiness and stuff
    T stack[N];
    int ct;
};

}
#endif /* __STACK_H */
