#ifndef LOCK_HPP
#define LOCK_HPP

#ifdef PIN_H
#define Lock           PIN_RWMUTEX
#define lock_init(l)   PIN_RWMutexInit(l)
#define lock_fini(l)   PIN_RWMutexFini(l)
#define lock_rdlock(l) PIN_RWMutexReadLock(l)
#define lock_wrlock(l) PIN_RWMutexWriteLock(l)
#define lock_unlock(l) PIN_RWMutexUnlock(l)    
#else
#include <pthread.h>
#define Lock           pthread_rwlock_t
#define lock_init(l)   pthread_rwlock_init(l, NULL)
#define lock_fini(l)   pthread_rwlock_destroy(l)
#define lock_rdlock(l) pthread_rwlock_rdlock(l)
#define lock_wrlock(l) pthread_rwlock_wrlock(l)
#define lock_unlock(l) pthread_rwlock_unlock(l)    
#endif

#endif
