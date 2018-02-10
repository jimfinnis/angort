/**
 * @file lock.h
 * @brief  Brief description of file.
 *
 */

#ifndef __LOCK_H
#define __LOCK_H

#if defined(ANGORT_POSIXLOCKS)
#include <pthread.h>
#endif

#define lockprintf if(0)printf
//#define lockprintf printf

namespace angort {

/// an rwlock thingy; don't want to require c++17 at this point.
/// Shared-lockable classes should inherit this, and then create either
/// ReadLock or WriteLock in the block where access is done (RAII).

class Lockable {
    friend class ReadLock;
    friend class WriteLock;
#if defined(ANGORT_POSIXLOCKS)
    pthread_rwlock_t lock;
    const char *lockablename;
    int writelockct;
#endif
public:
#if defined(ANGORT_POSIXLOCKS)
    const char *getLockableName(){return lockablename;}
#endif    
    Lockable(const char *n){
#if defined(ANGORT_POSIXLOCKS)
        writelockct=0;
        lockablename = n;
        lockprintf("Registering lockable %s at %p\n",lockablename,this);
        pthread_rwlock_init(&lock,NULL);
#endif
    }
    ~Lockable(){
#if defined(ANGORT_POSIXLOCKS)
        pthread_rwlock_destroy(&lock);
#endif
    }
};

class ReadLock {
    Lockable *t;
public:
    ReadLock(const Lockable* _t){
#if defined(ANGORT_POSIXLOCKS)
        t = (Lockable *)_t;
        if(t){
            lockprintf("READLOCK START on %s %p\n",t->getLockableName(),&t->lock);
            pthread_rwlock_rdlock(&t->lock);
        }
#endif
    }
    ~ReadLock(){
#if defined(ANGORT_POSIXLOCKS)
        if(t){
            lockprintf("READLOCK END on %s %p\n",t->getLockableName(),&t->lock);
            pthread_rwlock_unlock(&t->lock);
        }
#endif
    }
};

class WriteLock {
    Lockable *t;
public:
    WriteLock(const Lockable* _t){
#if defined(ANGORT_POSIXLOCKS)
        t = (Lockable *)_t;
        if(t){
            if(!t->writelockct)
                pthread_rwlock_wrlock(&t->lock);
            t->writelockct++;
            lockprintf("WRITELOCK START on %s %p, ct now %d\n",t->getLockableName(),&t->lock,t->writelockct);
        }
#endif
    }
    ~WriteLock(){
#if defined(ANGORT_POSIXLOCKS)
        if(t){
            t->writelockct--;
            lockprintf("WRITELOCK END on %s %p, ct now %d\n",t->getLockableName(),&t->lock,t->writelockct);
            if(!t->writelockct)
                pthread_rwlock_unlock(&t->lock);
        }
#endif
    }
};

/// will return true if the library was compiled with locking
bool hasLocking();
    

}

#endif /* __LOCK_H */
