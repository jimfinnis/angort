/**
 * @file lock.h
 * @brief  Brief description of file.
 *
 */

#ifndef __LOCK_H
#define __LOCK_H

#include <pthread.h>

/// an rwlock thingy; don't want to require c++17 at this point.
/// Shared-lockable classes should inherit this, and then create either
/// ReadLock or WriteLock in the block where access is done (RAII).

class Lockable {
    friend class ReadLock;
    friend class WriteLock;
    pthread_rwlock_t lock;
    const char *lockablename;
    int writelockct;
public:
    const char *getLockableName(){return lockablename;}
    
    Lockable(const char *n){
        writelockct=0;
        lockablename = n;
        pthread_rwlock_init(&lock,NULL);
    }
    ~Lockable(){
        pthread_rwlock_destroy(&lock);
    }
};

class ReadLock {
    Lockable *t;
public:
    ReadLock(const Lockable* _t){
        t = (Lockable *)_t;
//        printf("READLOCK START on %s %p\n",t->getLockableName(),&t->lock);
        pthread_rwlock_rdlock(&t->lock);
    }
    ~ReadLock(){
//        printf("READLOCK END on %s %p\n",t->getLockableName(),&t->lock);
        pthread_rwlock_unlock(&t->lock);
    }
};

class WriteLock {
    Lockable *t;
public:
    WriteLock(const Lockable* _t){
        t = (Lockable *)_t;
        if(!t->writelockct)
            pthread_rwlock_wrlock(&t->lock);
        t->writelockct++;
//        printf("WRITELOCK START on %s %p, ct now %d\n",t->getLockableName(),&t->lock,t->writelockct);
    }
    ~WriteLock(){
        t->writelockct--;
//        printf("WRITELOCK END on %s %p, ct now %d\n",t->getLockableName(),&t->lock,t->writelockct);
        if(!t->writelockct)
            pthread_rwlock_unlock(&t->lock);
    }
};

#endif /* __LOCK_H */
