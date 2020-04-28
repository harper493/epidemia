#ifndef __PREFETCHER
#define __PREFETCHER

#include "common.h"
#include "prefetch.h"
#include "aligned.h"
#include <boost/type_traits.hpp>

/************************************************************************
 *
 * Overview
 * --------
 *
 * A prefetch queue deals with getting everything prefetched into L1 cache
 * before a pipeline stage starts working on it. It can be thought of as a
 * kind of ladder. You put something (a buffer) in at the top. It falls down
 * one step at a time, and by the time it falls off the bottom, everything
 * has been prefetched and you can go to work on that buffer sure that 
 * everything you need is in L1 cache.
 *
 * The main interface to this class is the do_one() function, which takes
 * a buffer as an argument, and returns another one. Either can be NULL.
 * Each time the function is called, it does as described above, rippling 
 * the existing content down one level.
 *
 * If this particular pipeline stage is relatively idle, it's possible that
 * a buffer will get to the bottom without everything having been prefetched.
 * But in that case it doesn't matter. If the pipeline stage is busy,
 * there will be enough time between calls to do_one that the prefetch
 * at each stage will complete as needed. Hence there is a nice negative
 * feedback loop.
 *
 * Prefetch Function
 * -----------------
 *
 * The user class must define a prefetch function, passed as the template
 * argument PREFETCH. This takes three parameters:
 *
 * -- the pointer to the buffer structure it is to work on
 * -- the "ladder step" to do the work for
 * -- the phase of the ladder step. Usually this can be ignored;
 *    it is explained below
 *
 * Sometimes (often) a later prefetch stage depends on the result of
 * an earlier prefetch. For example, the first stage nearly always
 * fetches the buffer itself, containing pointers to other things
 * that need to be prefetched.
 *
 * A prefetch operation may take up to 200uS if the content has to
 * come all the way from DRAM. Therefore, sometimes a prefetch
 * stage needs to do nothing but wait.
 *
 * A good example of how this comes together is in flow_table::prefetch_bucket,
 * which needs to prefetch three things: the buffer, the first two cache
 * lines of the buffer's bucket, and the flow table hash bucket that the
 * buffer points to.
 *
 * Prefetching a structure, or part of it, is done using the utility
 * function prefetch(), which knows the size of the structure and hence the
 * number of cache lines, and generates the requisite number of prefetch
 * instructions. The function prefetch_n<N> can be used
 * to force a specific number of cache lines. Note that
 * all structures which are accessed in this way are cache-line aligned.
 *
 * Code generation
 * ---------------
 *
 * You might reasonably think, looking at the code here, in the prefetch functions,
 * and in the prefetch() function itself, that this would generate truly
 * awful code. But in fact the compiler is really smart. If you look at the
 * assembler output, with -O3 optimization, you will see that this deep
 * nest of functions turns into a straight line of the right instructions, as
 * good as or better than anything you would handcode yourself.
 *
 * Implementation
 * --------------
 *
 * The "obvious" way to implement this is as an array of pointers of the size
 * of the queue, with the pointers being rippled down at every do_one() call.
 * Turns out this is quite expensive to do. So instead we keep a much larger 
 * array and just let the active part drift upwards. When it gets near the top we 
 * relocate everything back down the base, and start over. This is also 
 * cheaper than allowing the content to wrap, since it avoids a bunch of
 * "have we wrapped yet" checks.
 *
  ***********************************************************************/

#define INSTANCES 16

template<class COLL, int N, void(PREFETCH)(typename COLL::value_type,int)>
class prefetcher : public cache_aligned
{
public:
    typedef prefetcher<COLL, N,PREFETCH> my_type;
    typedef typename COLL::value_type B;
private:
    size_t base = 0;
    B entries[(N+1)*INSTANCES];
    typename COLL::const_iterator my_iter;
    typename COLL::const_iterator end_iter;
public:
    prefetcher()
    {
        clear();
    }
    prefetcher(const COLL &coll)
    {
        clear();
        start(coll);
    }
    void clear()
    {
        for (size_t i=0; i<(N+1)*INSTANCES; ++i) {
            entries[i] = NULL;
        }
    }
    void start(const COLL &coll)
    {
        my_iter = coll.begin();
        end_iter = coll.end();
        for (int i=0; i<N; ++i) {
            B b;
            if (my_iter==end_iter) {
                b = do_one(NULL);
            } else {
                b = do_one(*my_iter++);
            }
            debug_assert(b==NULL);
        }
    }
    B get()
    {
        B result = NULL;
        if (my_iter!=end_iter) {
            result = do_one(*my_iter++);
        } else {
            result = do_one(NULL);
        }
        return result;
    }
    B do_one(B new_one) _always_inline
    {
        if (likely(new_one)) {
            PREFETCH(new_one, N-1);
        }
        entries[base+N] = new_one;
        B result = entries[base];
        for (size_t i=1; i<N; ++i) {
            B e = entries[base+i];
            if (e) {
                PREFETCH(e, i-1);
            }
        }
        ++base;
        if (unlikely(base > (INSTANCES-1)*(N+1))) {
            for (size_t i=0; i<N; ++i) {
                entries[i] = entries[base+i];
            }
            base = 0;
        }
        return result;
    }
    bool empty() const { return entries[base]==NULL; }
    class iterator : public std::forward_iterator_tag
    {
    public:
        typedef B value_type;
        typedef B& reference_type;
        typedef std::forward_iterator_tag iterator_category;
    private:
        my_type *my_prefetcher = NULL;
        B result = NULL;
    public:
        iterator() { };
        iterator(my_type *pf)
            : my_prefetcher(!pf->empty() ? pf : NULL),
              result(my_prefetcher ? my_prefetcher->get() : NULL)
        { };
        iterator(const iterator &other)
            : my_prefetcher(other.my_prefetcher), result(other.result)
        { };
        iterator &operator=(const iterator &other)
        {
            my_prefetcher = other.my_prefetcher;
            result = other.result;
            return *this;
        };
        bool operator==(const iterator &other) const { return my_prefetcher==other.my_prefetcher; };
        bool operator!=(const iterator &other) const { return !(*this==other); };
        value_type operator*() const { assert(result!=0); return result; };
        value_type operator->() const { assert(result!=0); return result; };
        iterator &operator++()
        {
            if (my_prefetcher) {
                result = my_prefetcher->get();
                if (result==NULL) {
                    my_prefetcher = NULL;
                }
            }
            return *this;
        }
        iterator operator++(int) { iterator result=*this; ++*this; return result; };
    };
    iterator begin() { return iterator(this); };
    iterator end() { return iterator(); };
} _cache_aligned;


#endif
