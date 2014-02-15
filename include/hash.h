#ifndef __MYHASH_H
#define __MYHASH_H

/**
 * @file
 * Hash : An implementation of a hash table keyed on a subset of Value
 * based on the Python dictionary. If you want a faster version
 * keyed on uint32_t, look in intkeyedhash.h. The code is pretty much the
 * same, but the uint32_t code has more optimisations in it, notably the
 * comparisions are (obviously) much simpler.
 * 
 * This is from Lana's code, largely.
 */

#include <stdlib.h>
#include <string.h>

#include "angort.h"

// 5 in original
#define PERTURB_SHIFT 5
// 32 in original
#define INITIAL_SIZE 32

//#define DEBUG 1


/// an individual slot in the hash table.

struct HashEnt {
    /// the key - an incRef() may be required
    Value k;
    /// the value - an incRef() may be required
    Value v;
    
    /// the hash calculated from the key
    uint32_t hash;
    
    /// the slot is initialised to free by setting the key value to None
    HashEnt(){k.init();}
    
    /// return whether this slot is actively used - i.e.
    /// is not deleted or unused.
    inline bool isUsed(){
        return k.t!=Types::tNone && k.t != Types::tDeleted;
    }
    
    /// return whether this slot is, and never has been, used.
    inline bool isFree(){
        return k.t == Types::tNone;
    }
    
    /// return whether this slot has been deleted.
    inline bool isDeleted(){
        return k.t == Types::tDeleted;
    }
};

/// This is an implementation of a hash table mapping Values to Values.
/// There also exists IntKeyedHash, which is a generic mapping uint32_t to 
/// anything.
///
/// Both implementations are based on the Python dictionary, from notes
/// in Beautiful Code. There's also a good description of the Python
/// implementation at http://www.laurentluce.com/?p=249
/// Also the original source code is in dictobject.c

class Hash {
public:
    
    Hash(){
#ifdef DEBUG
        miss=0;
#endif
        mask = INITIAL_SIZE-1;
        table = new HashEnt[mask+1];
        used=0;
        fill=0;
    }
    
    ~Hash(){
        int s = mask+1;
        HashEnt *ent = table;
        delete [] table;
#ifdef DEBUG
        //        fprintf(stderr,"misses : %d, size %d\n",miss,mask+1);
#endif
    }
    
    /// set a value in the table
    
    virtual void set(Value *k,Value *val){
        
        uint32_t hash = k->getHash();
        HashEnt *ent = look(k,hash);
        int n_used = used;
        
        // we use the type directly, it's a bit quicker than isUsed() et. al.
        Type *tp = ent->k.t;
        if(tp==Types::tNone || tp==Types::tDeleted) {
            // there wasn't a value there before
            if(tp==Types::tNone)
                fill++; //we aren't overwriting a dummy, so increment fill
            
            ent->k.copy(k); // store the key into the table
            ent->hash = hash;
            used++; // increment used
        }
        // store the value - copy ctor will run, doing the required incref 
        // and decref on previous value.
        ent->v.copy(val);
        
        //        if(ent->k.t==HashKeyString)
        //            dumprefsimplemalloc("set",ent->k.d.p);
        
        // if the hash has grown, and it's more full (including deleted slots)
        // than 2/3, resize. 
//        printf("used:%d nused:%d\n",used,n_used);
//        printf("fill*3:%d (mask+1)*2:%d\n",fill*3,(mask+1)*2);

        if((used > n_used && fill*3 >= (mask+1)*2))
            resize((used>50000 ? 2:4)*used);
    }
    
    /// finds a value in the hash table, returning true and setting
    /// the internal found value pointer if found. If found, the value
    /// can then be retrieved with getval().
    
    virtual bool find(Value *k){
        HashEnt *ent = look(k,k->getHash());
        if(ent->isUsed()) {
            storedVal = &ent->v;
            return true;
        }
        else
            return false;
    }
    
    /// get the last value found by find()
    Value *getval(){
        return storedVal;
    }
    
    /// delete an item with a given key, returning true if we did it
    bool del(Value *k) {
        HashEnt *ent = look(k,k->getHash());
        if(!ent->isUsed())
            return false;
        ent->k.clr();
        ent->k.t = Types::tDeleted;
        ent->v.clr();
        used--;
        
        //        if(fill>used*4)
        //            resize((used>50000 ? 2:4)*used);
	return true;
    }
    
    
    /// create a value iterator
    class Iterator<Value *> *createValueIterator();
    /// create a key iterator
    class Iterator<Value *> *createKeyIterator();
    
#ifdef DEBUG
    int miss;
#endif
    // I wish this lot could be private, but the template below needs to
    // see them. And I can't put a friend declaration for the template in,
    // because then that would have to be above this class. And *that* wouldn't work,
    // because it needs things in *this* class. The joy of templates.
    HashEnt *table;
    Value *storedVal; //!< last value fetched
    
    unsigned int used; //!< number of slots occupied by keys
    unsigned int fill; //!< number of slots occupied by keys or dummies (used only if we implement deletion)
    unsigned int mask; //!< hashtable contains mask+1 slots
    
    
    void resize(unsigned int minused){
        unsigned int oldsize = mask+1;
        HashEnt *oldtable = table;
        
        int newsize;
        for(newsize = oldsize; newsize<=minused && newsize>0;newsize<<=1){}
//        printf("resizing to %d\n",newsize);
        
        table = new HashEnt[newsize];
        mask = newsize-1;
        used = 0;
        fill = 0;
        // iterate values, reinserting into new table
        HashEnt *ent=oldtable;
        
        for(unsigned int i=0;i<oldsize;i++,ent++){
            if(ent->isUsed()){
                HashEnt *newent = look(&ent->k,ent->k.getHash());
                if(newent->k.t==Types::tNone){
                    fill++;
                    used++;
                    // shouldn't need to incref the keys
                    newent->k.copy(&ent->k);
                    newent->v.copy(&ent->v);
                    newent->hash = ent->hash;
                    
//                    printf("RESIZE SET\n");
                }
                else throw Exception("invalid in resize");
            }
        }
        delete [] oldtable;
//        printf("RESIZE END!\n");
    }
    
    /// scan, looking for either a slot with this key or the
    /// slot where this key would go
    
    HashEnt *look(Value *k,uint32_t hash){
        register unsigned int slot = hash & mask;
        register HashEnt *ent = table+slot;
        register HashEnt *freeslot;
        
        if(ent->isFree())
            return ent;
        if(ent->isDeleted())
            freeslot = ent;
        else if(ent->hash == hash && ent->k.equalForHashTable(k))
            return ent;
        else 
            freeslot = NULL;
        
        for(unsigned int perturb = hash;;perturb>>=PERTURB_SHIFT){
#ifdef DEBUG
            miss++;
#endif
            slot = (slot<<2)+slot+1+perturb;
            ent = table+(slot&mask);
            if(ent->isFree())
                return freeslot==NULL ? ent : freeslot;
            if(!ent->isDeleted() && ent->k.equalForHashTable(k))
                return ent;
            else if(ent->isDeleted() && freeslot==NULL)
                freeslot = ent;
        }
    }
};

/// Hash iterator - you probably won't access this
/// directly.

class HashValueIterator : public Iterator<Value *> {
public:
    HashValueIterator(Hash *h){
        hash = h;
        ent = NULL;
    }
    
    virtual void first(){
        size=hash->mask+1;
        ent=hash->table;
        idx=0;
        while(idx<size && !ent->isUsed()) {idx++;ent++;}
    }
    virtual void next(){
        ent++;idx++;
        while(idx<size && !ent->isUsed()) {idx++;ent++;}
    }
    virtual bool isDone() const {
        return idx>=size;
    }
    virtual Value *current() {
        if(!ent)
            throw Exception("first() not called on iterator");
        return &ent->v;
    }
private:
    Value key; //!< temporary for currentKey()
    Hash *hash;
    int idx,size;
    HashEnt *ent;
};

/// Hash iterator - you probably won't access this
/// directly.

class HashKeyIterator : public Iterator<Value *> {
public:
    HashKeyIterator(Hash *h){
        hash = h;
        ent = NULL;
    }
    
    virtual void first(){
        size=hash->mask+1;
        ent=hash->table;
        idx=0;
        while(idx<size && !ent->isUsed()) {idx++;ent++;}
    }
    virtual void next(){
        ent++;idx++;
        while(idx<size && !ent->isUsed()) {idx++;ent++;}
    }
    virtual bool isDone() const {
        return idx>=size;
    }
    virtual Value *current() {
        if(!ent)
            throw Exception("first() not called on iterator");
        return &ent->k;
    }
private:
    Hash *hash;
    int idx,size;
    HashEnt *ent;
};

inline Iterator<Value *> *Hash::createKeyIterator() {
    return new HashKeyIterator(this);
};

inline Iterator<Value *> *Hash::createValueIterator() {
    return new HashValueIterator(this);
};




#endif /* __MYHASH_H */

