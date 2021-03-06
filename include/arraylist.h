/** @file
 * ArrayList, An array list implementation
 */

#ifndef __ANGORTARRAYLIST_H
#define __ANGORTARRAYLIST_H

// we need placement new, sadly.
#include <new> 

#include "lock.h"

namespace angort {

/// comparator object for sorting
template <class T> struct ArrayListComparator {
    virtual int compare(const T *a, const T *b) = 0;
};
    

/// array list with random access get at  O(1). Insertion at O(n)
/// except at the end of the list where it's O(1). List will occasionally
/// resize, more often on grow than on shrink. Docs need improving.
/// Another hideousness - we actually use malloc and free here,
/// because of the resizing issue. Bad things happen to garbage
/// counts when the system resizes the array; we want resizing to 
/// not run the destructors and constructors. We do run these
/// when the list is created or destroyed, however.

template <class T> class ArrayList : public Lockable {
    // because I don't want to rely on c++11 delegated ctors
    void init(int n){ 
        capacity = n;
        baseCapacity = n;
        ct = 0;
        data = (T*)malloc(sizeof(T)*n);
        locks=0;
        // don't run constructors until there are items there!
    }
public:
    /// create a list, with initially enough room for n elements
    ArrayList(int n) : Lockable("list"){
        init(n);
    }
    
    ArrayList() : Lockable("list"){
        init(4);
    }
    
    /// destroy a list
    ~ArrayList() {
        // inplace destruction of only those items
        // which exist
        for(int i=0;i<ct;i++)
            data[i].~T();
        free(data);
        
    }
    
    /// add an item to the end of the list, return a pointer to
    /// fill in the item. Runs in O(1) time unless the list needs
    /// resizing.
    T *append(){
        reallocateifrequired(ct+1);
        new (data+ct) T(); // inplace construction of new item
        return data+(ct++);
    }
    
    /// insert an item before position n, returning a pointer to
    /// fill in the item. Runs in O(n) time unless n==-1, in which
    /// case it's O(1)
    T *insert(int n=-1){
        if(n<0 || n>=ct)
            return append();
        reallocateifrequired(ct+1);
        memmove(data+n+1,data+n,(ct-n)*sizeof(T));
        ct++;
        new (data+n) T(); // inplace construction of new item
        return data+n;
    }
    
    /// remove an item from somewhere in the list in O(n) time
    bool remove(int n=-1){
        if(n<0||n>=ct)
            return false;
        ct--;
        // destruct the item we're about to remove
        data[n].~T();
        if(n!=ct)
            memmove(data+n,data+n+1,(ct-n)*sizeof(T));
        reallocateifrequired(ct-1);
        return true;
    }
    
    /// get a pointer to the nth item of a list in O(1) time. If n==-1
    /// will return the last item.
    T *get(int n){
        if(n<0||n>=ct)
            throw RUNT(EX_OUTOFRANGE,"list get out of range");
        return data+n;
    }
    
    /// clear the entire list, does not run
    /// destructors, just sets the size to zero.
    void clear(){
        ct=0;
    }
    
    /// return the size of the list
    int count(){
        return ct;
    }
    
    /// return the capacity of the list
    int getCapacity(){
        return capacity;
    }
    
    /// set a value in the list
    void set(int n,T *v){
        if(locks)
            throw RUNT(EX_MODITER,"cannot modify list as it is iterated");
        if(n<0)
            throw RUNT(EX_OUTOFRANGE,"list set index out of range");
        if(n>=ct){
            reallocateifrequired(n+10); // allocate a bit more
            // initialise the new values!
            for (int i=ct;i<capacity;i++){
                new (data+i)T();
            }
            ct=n+1;
        }
        data[n].copy(v);
    }
    
    /// get a slot to copy a value into
    T *set(int n){
        if(n<0)
            throw RUNT(EX_OUTOFRANGE,"list set index out of range");
        if(n>=ct){
            reallocateifrequired(n+10); // allocate a bit more
            // initialise the new values!
            for (int i=ct;i<capacity;i++){
                new (data+i)T();
            }
            ct=n+1;
        }
        return data +n;
    }
    
    /// get the index of an item in the list
    int getIndexOf(T *v){
        int n = v-data;
        if(n<0 || n>=ct)
            throw RUNT(EX_OUTOFRANGE,"get index of item returns out of range");
        return n;
    }
    
    /// sort in place - requires specialisation
    void sort(ArrayListComparator<T> *cmp);

    /// number of locks held - these are added by iterators
    int locks;

private:
    
    /// reallocate the list if required by the given new count and copy
    /// all items over. Will NOT change ct.
    void reallocateifrequired(int newct){
        T *newdata;
        if(newct>=capacity){
            // need to grow the list
//            printf("oldct %d, newct %d, cap %d\n",ct,newct,capacity);
            capacity = newct + (newct>>3) + (newct<9?3:6);
        } else if(capacity>baseCapacity && newct<(capacity>>1)) {
            // need to shrink the list. New capacity should still
            // have at least one empty space left at the end, for popped
            // items!
//            printf("oldct %d, newct %d, cap %d\n",ct,newct,capacity);
            capacity = capacity>>1;
//            printf("SHRINK to %d\n",capacity);
        } else
            return;
        
        // do the resize; don't run ctors - they've already been run on
        // the data
        newdata = (T*)malloc(sizeof(T)*capacity);
        memcpy(newdata,data,sizeof(T)*ct);
        free(data); // without running dtors because they've been moved
        data = newdata;
    }
    
    /// the data area
    T *data;
    /// the number of items currently stored in the list
    int ct;
    /// the capacity of the list
    int capacity;
    /// the initial capacity of the list, lower than which we never go
    int baseCapacity;
};

template <class T> class ArrayListIterator : Iterator<T *> {
public:
    ArrayListIterator(ArrayList<T> *a){
        idx=-1;
        list = a;
        list->locks++;
    }
    
    virtual ~ArrayListIterator(){
        list->locks--;
    }
    
    virtual void first(){
        idx = 0;
    }
    virtual void next(){
        idx++;
    }
    virtual bool isDone() const {
        return idx>=list->count();
    }
    virtual int index() const {
        return idx;
    }
    virtual T *current() {
        return list->get(idx);
    }
    
    int getIdx() {
        return idx;
    }

protected:
    int idx;
    ArrayList<T> *list;
};

template<> void ArrayList<Value>::sort(ArrayListComparator<Value> *cmp);

}


#endif /* __ARRAYLIST_H */
