#ifndef __PREFETCH
#define __PREFETCH

#include "common.h"

/************************************************************************
 * Prefetch a structure. We use compile-time recursion to prefetch
 * it in cache-line sized chunks.
 *
 * prefetch(obj) - prefetches the entire object
 * prefetch<n>(obj) - prefetches the first n cache lines of obj
 * prefetch_upto(obj, member) - prefetches the first part of obj up
 *                              to but including member
 * prefetch_from_to(obj, m1, m2) - prefetches the part
 *                                 of obj between m1 and m2
 ***********************************************************************/

template<int N>
struct prefetchee
{
    char fill[N*CACHE_LINE_SIZE];
} _cache_aligned;

template<int N>
void _prefetch(const void *c) _always_inline;

template<class C>
void prefetch(const C *c) _always_inline;

template<int N>
inline void _prefetch(const void *c)
{
    _prefetch<N-1>(c);
    __builtin_prefetch(((const char*)c) + CACHE_LINE_SIZE*(N-1));
}

template<>
inline void _prefetch<0>(const void *c)
{
}

template<class C>
inline void prefetch(const C *c)
{
    _prefetch<CACHE_LINES_IN(sizeof(C))>(c);
}

template<int N>
inline void prefetch_n(const void *c)
{
    _prefetch<N>(c);
}

#define prefetch_upto(CLASS, c, upto) \
    _prefetch<CACHE_LINES_IN(__builtin_offsetof(CLASS, upto))>(c);

#define prefetch_from_to(CLASS, c, from, to) \
    _prefetch<CACHE_LINES_IN(__builtin_offsetof(CLASS, to)-__builtin_offsetof(CLASS, from))>(&(c->from));

#endif

