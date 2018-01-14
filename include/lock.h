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
public:
    Lockable(){
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
        pthread_rwlock_rdlock(&t->lock);
    }
    ~ReadLock(){
        pthread_rwlock_unlock(&t->lock);
    }
};

class WriteLock {
    Lockable *t;
public:
    WriteLock(const Lockable* _t){
        t = (Lockable *)_t;
        pthread_rwlock_wrlock(&t->lock);
    }
    ~WriteLock(){
        pthread_rwlock_unlock(&t->lock);
    }
};

#endif /* __LOCK_H */
