
#ifndef __ATOMIC
#define __ATOMIC

#include "common.h"

template<class T, class T2, class T3>
bool atomic_cas(T &dst, T2 prior, T3 post)
{
    bool result = false;
    result = __sync_bool_compare_and_swap(&dst, prior, post);
    return result;
}

template<class T, class T2>
inline T atomic_swap(T &dst, T2 src)
{
    return __sync_lock_test_and_set(&dst, src);
}

template<class T>
inline void atomic_release(T &lock)
{
    __sync_lock_release(&lock);
}

template<class T, class T2>
inline void atomic_add(T &addend, T2 value)
{
    __sync_add_and_fetch(&addend, value);
};

template<class T, class T2>
inline T atomic_add_and_fetch(T &addend, T2 value)
{
    return __sync_add_and_fetch(&addend, value);
}

template<class T, class T2>
inline void atomic_subtract(T &addend, T2 value)
{
    __sync_sub_and_fetch(&addend, value);
}

template<class T, class T2>
inline T atomic_subtract_get(T &addend, T2 value)
{
    return __sync_sub_and_fetch(&addend, value);
}

template<class T, class T2>
inline T atomic_cyclic_increment(T &addend, T2 limit)
{
    T result;
    while (true) {
        result = __sync_fetch_and_add(&addend, 1);
        if (addend>=limit) {
            atomic_cas(addend, result+1, 0);
        } else {
            break;
        }
    }
    return result;
}

#endif
