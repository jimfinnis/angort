/** @file
 * This file contains the cycle manager, which itself contains the 
 * global container list, which contains
 * all entities present in the system which can refer to other
 * objects. This is used during the occasional garbage collect to detect
 * cycles of reference not referenced from outside. These entities
 * are deleted.
 */

#ifndef __CYCLE_H
#define __CYCLE_H

/// doubly-linked list of GarbageCollected objects

namespace angort {

class GCList {
    friend class GarbageCollected;
public:
    
    GCList(){
        reset();
    }
    
    /// empty the list
    
    void reset()
    {
        ct = 0;
        hd = tl = NULL;
    }
    
    
    /// return the number of entries in the list
    
    int entries() { return ct; }
    
    /// don't use this in live code, it's slow as thing.
    bool isInList(GarbageCollected *o){
        GarbageCollected *p;
        for(p=hd;p;p=p->next){
            if(p==o)return true;
        }
        return false;
    }
    
    /// add an item to the start of the list
    void addToHead(GarbageCollected *o)
    {
#ifdef DEBUG
        if(isInList(o))
            throw Exception("item added twice to GC list");
#endif
        ct++;
        if(hd)
        {
            hd->prev = o;
            o->prev = NULL;
            o->next = hd;
            hd = o;
        }
        else
        {
            hd = tl = o;
            o->next = o->prev = NULL;
        }
    }
    
    /// add an item to the end of the list
    void addToTail(GarbageCollected *o)
    {
#if 1
        if(isInList(o))
            throw Exception("item added twice to GC list");
#endif
        ct++;
        if(hd)
        {
            tl->next = o;
            o->next = NULL;
            o->prev = tl;
            tl = o;
        }
        else
        {
            hd = tl = o;
            o->next = o->prev = NULL;
        }
    }
    
    /// remove an item from the list
    void remove(GarbageCollected *p)
    {
        ct--;
        if(p->prev)
            p->prev->next = p->next;
        else
            hd = p->next;
        if(p->next)
            p->next->prev = p->prev;
        else
            tl = p->prev;
    }
    
    
    /// return the head of the list
    GarbageCollected *head() { return hd; }
    
    /// return the tail of the list
    GarbageCollected *tail() { return tl; }
    
    /// get next item of a given item
    GarbageCollected *next(GarbageCollected *p) {
        return p->next;
    }
    
    /// get prev item of a given item
    GarbageCollected *prev(GarbageCollected *p) {
        return p->prev;
    }
    
    /// copy a list from another
    void copy(GCList &list)
    {
        hd = list.hd;
        tl = list.tl;
        ct = list.ct;
    }
    
protected:
    GarbageCollected *hd;	//!< the head of the list
    GarbageCollected *tl; //!< the tail of the list
    int ct;	//!< the number of entries in the list
};

/// this class contains a list of all the container objects - objects
/// which can contain references to other objects. It can detect
/// reference cycles within the objects in the list and destroy them.
/// 
/// The algorithm used is described in http://arctrix.com/nas/python/gc/
///

class CycleDetector {
    /// initialise the cycle detector, clearing the list.
    CycleDetector(){
        inDeleteCycle=false;
        mainlist.reset();
    }
    static CycleDetector *instance;
public:
    
    /// singleton instance fetcher
    static CycleDetector *getInstance(){
        if(!instance)
            instance = new CycleDetector();
        return instance;
    }
    
    
    /// add an item to the cycle detector's list. This will
    /// be any item which can hold a reference to another item.
    
    void add(GarbageCollected *o){
//        printf("ADDING %p\n",o);
        mainlist.addToTail(o);
    }
    
    /// remove an item from the cycle detector's list, called when
    /// the item is destroyed.
    
    void remove(GarbageCollected *o){
//        printf("REMOVING %p\n",o);
        mainlist.remove(o);
    }
    
    /// actually detect cycles and delete objects locked in a cycle
    /// which are not referred to from elsewhere.
    void detect();
    
    /// return the number of containers in the system
    int count(){
        return mainlist.entries();
    }
    
    /// move an item from the mainlist to the newlist
    void move(GarbageCollected *gc) {
        if(gc->gc_refs==0){
            dprintf("    MOVE %p into new list\n",gc);
            mainlist.remove(gc);
            newlist.addToTail(gc);
            gc->gc_refs=1;
        }
    }
    
    /// move the entity, and the items referenced by it, into the newlist if appropriate
    void traceAndMoveEntity(GarbageCollected *p){
      move(p);
      traceAndMoveIterator(p,true);
      traceAndMoveIterator(p,false);
      p->traceAndMove(this);
    }
    
    /// show reference counts of all objects
    void dump();
        
    /// decrements the reference counts of all objects referred to in the iterable
    void decIteratorReferentsCycleRefCounts(GarbageCollected *gc,bool iskey);
    /// trace all collectable entities reachable from this iterable
    /// If their gc_refs is zero (i.e. still in the mainlist) move
    /// to the newlist
    void traceAndMoveIterator(GarbageCollected *gc,bool iskey);
    
    /// deletion prepwork - clears all references to objects marked - see detect()
    void clearZombieReferencesIterator(GarbageCollected *gc,bool iskey);
    
    /// check this to see if we should recursively delete in closures
    bool isInDeleteCycle(){
        return inDeleteCycle;
    }

private:
    /// the list of items
    GCList mainlist;
    /// the list of items we build in the process of GC - becomes the new main list
    GCList newlist;
    
    /// we set this when we're doing the final delete; it's used to stop closure
    /// deletion recursion being a problem. Hopefully.
    bool inDeleteCycle;
};
    
}
#endif /* __CYCLE_H */
