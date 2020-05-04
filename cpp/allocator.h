#ifndef __ALLOCATOR
#define __ALLOCATOR

#include "common.h"
#include "spinlock.h"

const int CHUNK_SIZE = 8192;

template<class C>
class allocator;

template<class C>
allocator<C> *get_allocator()
{
    return NULL;
}

template<class C>
class allocator
{
public:
    typedef allocator<C> my_type;
    struct chunk : bintr::list_base_hook<>
    {        
        void *next_free = &memory;
        size_t allocated = 0;
        array<U32, CHUNK_SIZE + CHUNK_SIZE * sizeof(C) / sizeof(U32)> memory;
        typedef bintr::list<chunk> list;
    };
private:
    typename chunk::list chunks;
    static __thread chunk *current_chunk;
    spinlock my_lock;
public:
    //    chunk *&get_current_chunk();
    C *alloc()
    {
        C *result = NULL;
        chunk *&c = current_chunk;
        if (!(c && c->allocated < CHUNK_SIZE)) {
            void *mem;
            int rc = posix_memalign(&mem, CACHE_LINE_SIZE, sizeof(chunk));
            if (rc==0) {
                c = new chunk();
                spinlock::scoped_lock sl(my_lock);
                chunks.push_back(*c);
            } else {
                c = 0;
            }
        }
        if (c) {
            result = reinterpret_cast<C*>(c->next_free);
            *(C**)(&c->next_free) += 1;
            c->allocated += 1;
        }
        return result;
    }
public:
};

template<class C>
class allocator_user
{
public:
    void *operator new(size_t size) 
    {        
        void *result = get_allocator<C>()->alloc(); 
        if (result==NULL) {
            throw std::bad_alloc();
        }
        return result;
    }
    void *operator new(size_t size, const std::nothrow_t&) noexcept
    { 
        return get_allocator<C>()->alloc(); 
    }
    void *operator new(size_t size, void *p) noexcept
    {
        return p;
    }
    void operator delete(void *ptr)
    {
    }
};


#define DEFINE_ALLOCATOR(CLASS)                                         \
    template<>                                                          \
    __thread allocator<CLASS>::chunk *allocator<CLASS>::current_chunk = NULL; \
    allocator<CLASS> CLASS##__allocator;                                \
    template<>                                                          \
    allocator<CLASS> *get_allocator()                                   \
    {                                                                   \
        return &CLASS##__allocator;                                     \
    }

#define DECLARE_ALLOCATOR(CLASS)                \


#endif
