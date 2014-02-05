#ifndef __ARRAYLIST_H
#define __ARRAYLIST_H

/** @file
 * ArrayList, An array list implementation
 */

/// array list exception - typically get out of range or pop on empty list
class ArrayListException : public Exception {
public:
    ArrayListException(const char *e) : Exception(e) {}
};

/// array list with random access get at  O(1). Insertion at O(n)
/// except at the end of the list where it's O(1). List will occasionally
/// resize, more often on grow than on shrink. Docs need improving :)

template <class T> class ArrayList {
public:
    /// create a list, with initially enough room for n elements
    ArrayList(int n){
        capacity = n;
        ct = 0;
        data = new T [n];
    }
    
    /// destroy a list
    ~ArrayList() {
        delete [] data;
    }
    
    /// add an item to the end of the list, return a pointer to
    /// fill in the item. Runs in O(1) time unless the list needs
    /// resizing.
    T *append(){
        reallocateifrequired(ct+1);
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
        return data+n;
    }
    
    /// remove an item from the list, runs in O(1) time, unless there's a resize
    T *pop(){
        if(!ct) throw ArrayListException("pop on empty list");
        // make DAMN SURE we still have the old item!
        reallocateifrequired(ct-1);
        ct--;
        return data+ct;
    }
    
    /// peek an item from the list, runs in O(1) time
    T *peek(){
        if(!ct) throw ArrayListException("peek on empty list");
        // make DAMN SURE we still have the old item!
        return data+(ct-1);
    }
    
    
    /// remove an item from somewhere in the list in O(n) time
    bool remove(int n=-1){
        if(n<0||n>=ct)
            return false;
        data[n].clr();
        reallocateifrequired(ct-1);
        ct--;
        if(n>=0 && n!=ct)
            memmove(data+n,data+n+1,(ct-n)*sizeof(T));
        return true;
    }
    
    /// get a pointer to the nth item of a list in O(1) time. If n==-1
    /// will return the last item.
    T *get(int n){
        if(n<0||n>=ct)
            throw ArrayListException("get out of range");
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
        if(n>=ct){
            reallocateifrequired(n+10); // allocate a bit more
            for(int i=ct;i<=capacity;i++){
                data[i].init();
            }
            ct=n+1;
        }
        data[n].copy(v);
    }
    
    /// get the index of an item in the list
    int getIndexOf(T *v){
        int n = v-data;
        if(n<0 || n>=ct)
            throw ArrayListException("getIndexOf() out of range");
        return n;
    }

private:
    
    /// reallocate the list if required by the given new count and copy
    /// all items over. Will NOT change ct.
    void reallocateifrequired(int newct){
        printf("Reallocate: capacity currently %d\n",capacity);
        T *newdata;
//        printf("ct %d, cap %d\n",newct,capacity);
        if(newct>=capacity){
            // need to grow the list
            capacity = newct + (newct>>3) + (newct<9?3:6);
            printf("GROW to %d\n",capacity);
        } else if(capacity>16 && newct<(capacity>>1)) {
            // need to shrink the list. New capacity should still
            // have at least one empty space left at the end, for popped
            // items!
            capacity = capacity>>1;
            printf("SHRINK to %d\n",capacity);
        } else
            return;
        
        // do the resize
        newdata = new T [capacity];
        memcpy(newdata,data,sizeof(T)*ct);
        delete [] data;
        data = newdata;

    }
    
    /// the data area
    T *data;
    /// the number of items currently stored in the list
    int ct;
    /// the capacity of the list
    int capacity;
    
};

template <class T> class ArrayListIterator : Iterator<T *> {
public:
    ArrayListIterator(ArrayList<T> *a){
        idx=-1;
        list = a;
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


#endif /* __ARRAYLIST_H */
