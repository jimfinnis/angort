/**
 * @file lock.h
 * @brief  Brief description of file.
 *
 */

#ifndef __LOCK_H
#define __LOCK_H

#include "config.h"


#define LOCKDEBUG 0

#if ANGORT_POSIXLOCKS
#pragma message "Threading on"
#include <pthread.h>
#include <errno.h>
#endif

#include "exception.h"

#if(LOCKDEBUG)
#define lockprintf if(boojum)printf
#else
#define lockprintf if(0)printf
#endif

namespace angort {

extern bool boojum;
/// an rwlock thingy; don't want to require c++17 at this point.
/// Shared-lockable classes should inherit this, and then create either
/// ReadLock or WriteLock in the block where access is done (RAII).

class Lockable {
    friend class ReadLock;
    friend class WriteLock;
#if ANGORT_POSIXLOCKS
    pthread_rwlock_t lock;
#endif
protected:
    const char *lockablename;
public:
#if ANGORT_POSIXLOCKS
    const char *getLockableName(){return lockablename;}
#endif    
    Lockable(const char *n){
#if ANGORT_POSIXLOCKS
        lockablename = n;
        lockprintf("Registering lockable %s at %p\n",lockablename,this);
        pthread_rwlock_init(&lock,NULL);
#endif
    }
    ~Lockable(){
#if ANGORT_POSIXLOCKS
        pthread_rwlock_destroy(&lock);
#endif
    }
};

/// done so we can call it with a default ctor, if we call lock()
class ReadLock {
    Lockable *t;
public:
    ReadLock(){
        t=NULL;
    }
    
    void lock(Lockable *_t){
#if ANGORT_POSIXLOCKS
        t = _t;
        t = (Lockable *)_t;
        if(t){
            lockprintf("READLOCK START on %s %p\n",t->getLockableName(),&t->lock);
            pthread_rwlock_rdlock(&t->lock);
        }
#endif
    }
    
    ReadLock(const Lockable* _t){
#if ANGORT_POSIXLOCKS
        lock(_t);
#endif
    }
    
    ~ReadLock(){
#if ANGORT_POSIXLOCKS
        if(t){
            lockprintf("READLOCK END on %s %p\n",t->getLockableName(),&t->lock);
            pthread_rwlock_unlock(&t->lock);
        }
#endif
    }
};

#define WL(a) WriteLock(a,__FILE__,__LINE__)


class WriteLock {
#if(LOCKDEBUG)
    const char *file;
    int line;
#endif
    Lockable *t;
    bool locked;
public:
    WriteLock(const Lockable* _t,const char *f,int l){
#if(LOCKDEBUG)
        file = f;
        line = l;
#endif
#if ANGORT_POSIXLOCKS
        t = (Lockable *)_t;
#if(LOCKDEBUG)
        lockprintf("%8lu: TRY WRITELOCK START on %s %p at %s:%d\n",pthread_self(),t->getLockableName(),&t->lock,file,line);
#endif
        if(t){
            if(pthread_rwlock_wrlock(&t->lock)==EDEADLK){
#if(LOCKDEBUG)
                lockprintf("%8lu:   WRITELOCK RELOCK OK\n",pthread_self());
#endif
                locked=false;
            } else {
#if(LOCKDEBUG)
                lockprintf("%8lu:   WRITELOCK START OK\n",pthread_self());
#endif
                locked=true;
            }
        } else locked=false;
#endif
    }
    ~WriteLock(){
#if ANGORT_POSIXLOCKS
        if(locked){
#if(LOCKDEBUG)
            lockprintf("%8lu: WRITELOCK END on %s %p at %s:%d\n",pthread_self(),t->getLockableName(),&t->lock,file,line);
#endif
            pthread_rwlock_unlock(&t->lock);
        } else {
#if(LOCKDEBUG)
            lockprintf("%8lu: WRITELOCK RELOCK END on %s %p at %s:%d\n",pthread_self(),t->getLockableName(),&t->lock,file,line);
#endif
        }
#endif
    }
};

/// will return true if the library was compiled with locking
bool hasLocking();
    

}

#endif /* __LOCK_H */
