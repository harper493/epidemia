#include "common.h"
#include "atomic.h"
#include <time.h>
#include <pthread.h>
#include <emmintrin.h>

#ifndef __SPINLOCK
#define __SPINLOCK

void _spinlock_wait() _always_inline;

inline void _spinlock_wait()
{
    _mm_pause();
}


class spinlock
{
public:
    class scoped_lock
    {
    private:
        spinlock *my_lock;
    public:
        scoped_lock() : my_lock(NULL) { };
        scoped_lock(spinlock &l) : my_lock(&l)
        {
            l.lock(NULL);
        };
        scoped_lock(spinlock &l, int no_use) : my_lock(&l)
        {
            l.try_lock(NULL);
        };
        ~scoped_lock() 
        { 
            release();
        };
        void acquire(spinlock &l) 
        {
            debug_assert(my_lock==NULL);
            my_lock = &l;
        }
        void release()
        {
            if (my_lock) {
                my_lock->release(); 
                my_lock = NULL;
            }
        }
    };
private:
    volatile int my_lock = 0;
public:
    spinlock() : my_lock(0) { };
    void lock() _always_inline
    {
        while(atomic_swap<volatile int, int>(my_lock, 1)) {
            while(my_lock) {
                _spinlock_wait();
            }
        }
    }
    void lock(void *c) _always_inline
    {
        lock();
    }
    bool try_lock() _always_inline
    {
        int old_val = atomic_swap<volatile int, int>(my_lock, 1);
        return old_val ? false : true;
    }
    bool try_lock(void *c) _always_inline
    {
        return try_lock();
    }
    void release() _always_inline
    {
        /*
         * Call atomic_release<volatile int>(my_lock) as it is an implicit
         * store barrier. 
         */
        atomic_release<volatile int>(my_lock);
    }
    void debug_assert_held()
    {
    }
    void unlock() _always_inline
    {
        release();              // to keep lockable class happy
    }
public:
};


/************************************************************************
 * shared_spinlock - A simple shared lock made with a single spinlock.
 * This lock sacrifices accounting and lock protocol enforcement (such as
 * ensuring that a thread already has the read lock before trying to upgrade
 * to the write lock) for performance.  Its up to the caller to respect the
 * shared lock protocol.
 ***********************************************************************/

class shared_spinlock
{
public:
    class scoped_lock
    {
    private:
        shared_spinlock *my_lock;
    public:
        scoped_lock() : my_lock(NULL) { };
        scoped_lock(shared_spinlock &l, bool share=true) : my_lock(&l)
        { 
            _do_lock(share);
        }
        ~scoped_lock() 
        { 
            if (my_lock) {
                my_lock->unlock_any();
            }
        }
        void acquire(shared_spinlock &l, bool share=true) 
        {
            debug_assert(my_lock==NULL);
            my_lock = &l;
            _do_lock(share);
        }
        bool upgrade()
        {
            debug_assert(my_lock && my_lock->is_shared());
            return my_lock->upgrade();
        }
        bool try_upgrade()
        {
            debug_assert(my_lock && my_lock->is_shared());
            return my_lock->try_upgrade();
        }
    private:
        void _do_lock(bool share)
        {
            if (share) {
                my_lock->lock_shared(); 
            } else {
                my_lock->lock();
            }
        }
    };
private:
    U16 writers = 0; // should only be 0 or 1
    U32 readers = 0;
    pthread_spinlock_t my_lock;
public:
    shared_spinlock()
    {
        pthread_spin_init(&my_lock, PTHREAD_PROCESS_PRIVATE);
        writers = 0;
        readers = 0;
    };
    ~shared_spinlock()
    {
        pthread_spin_destroy(&my_lock);
    };
    inline bool try_lock_shared()
    {
        pthread_spin_lock(&my_lock);
        if (writers > 0) {
            // fail
            pthread_spin_unlock(&my_lock);
            return false;
        } else {
            //success
            readers++;
            pthread_spin_unlock(&my_lock);
            return true;
        }
    };
    inline void lock_shared() {
        while (!try_lock_shared()) {
            _spinlock_wait();
        }
    }
    inline bool try_upgrade()
    {
        debug_assert(readers > 0);
        pthread_spin_lock(&my_lock);
        if (readers > 1 || writers > 0) {
            // fail
            pthread_spin_unlock(&my_lock);
            return false;
        } else {
            // success
            writers++;
            readers--;
            pthread_spin_unlock(&my_lock);
            return true;
        }
    };
    inline bool upgrade() {
        while (!try_upgrade()) {
            if (readers>1) {
                return false;
            }
            _spinlock_wait();
        }
        return true;
    };
    inline bool try_lock() 
    {
        pthread_spin_lock(&my_lock);
        if (readers > 0 || writers > 0) {
            // fail
            pthread_spin_unlock(&my_lock);
            return false;
        } else {
            // success
            writers++;
            pthread_spin_unlock(&my_lock);
            return true;
        }
    };
    inline void lock()
    {
        while (!try_lock()) {
            _spinlock_wait();
        }
    }
    inline bool is_shared() const // must only be called by a lock-holder
    {
        return writers==0;
    }
    inline void unlock_and_lock_shared()
    {
        debug_assert(writers > 0);
        pthread_spin_lock(&my_lock);
        readers++;
        writers--;
        pthread_spin_unlock(&my_lock);
    };
    inline void unlock()
    {
        debug_assert(writers > 0);
        pthread_spin_lock(&my_lock);
        writers--;
        pthread_spin_unlock(&my_lock);
    };
    inline void unlock_shared()
    {
        debug_assert(readers > 0);
        pthread_spin_lock(&my_lock);
        readers--;
        pthread_spin_unlock(&my_lock);
    };
    inline void unlock_any()
    {
        pthread_spin_lock(&my_lock);
        if (writers>0) {
            writers--;
        } else {
            readers--;
        }
        pthread_spin_unlock(&my_lock);
    };
friend class scoped_lock;
};


/************************************************************************
 * spinlock_conditional_scoped_lock - a scoped lock iff the template
 * parameter is true, a no-op if it is false.
 ***********************************************************************/

template<bool DO_LOCK=true>
class spinlock_conditional_scoped_lock : public spinlock::scoped_lock
{
public:
    spinlock_conditional_scoped_lock(spinlock &lock)
        : spinlock::scoped_lock(lock) { };
};

template<>
class spinlock_conditional_scoped_lock<false>
{
public:
    spinlock_conditional_scoped_lock(spinlock &lock) { };
};

#endif
