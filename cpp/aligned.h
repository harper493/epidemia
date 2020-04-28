
/************************************************************************
 * Copyright (C) 2017 Saisei Networks Inc. All rights reserved.
 ***********************************************************************/

#ifndef __ALIGNED
#define __ALIGNED

#include "common.h"
#include <stdlib.h>

extern void memtrack_record(void *c, void *m, U32 s);
/************************************************************************
 * Mixin base class for classes whose allocation should be aligned
 * on a given boundary. Template parameter is the alignment size.
 ***********************************************************************/

struct aligned_traits
{
};

template<int ALIGN>
class aligned : public aligned_traits
{
public:
    void *operator new(size_t size)
    {
        void *result = NULL;
        int r = posix_memalign(&result, ALIGN, size);
        if (result==NULL) {
            throw std::bad_alloc();
        }
        return result;
    }
    void *operator new(size_t size, const std::nothrow_t&)
    {
        void *result = NULL;
        int r = posix_memalign(&result, ALIGN, size);
        return result;
    }
    void *operator new(size_t, void *place) // placement new operator
    {
        return place;
    }
    void operator delete(void *ptr)
    {
        free(ptr);
    }
    enum { alignment = ALIGN };
};

typedef aligned<CACHE_LINE_SIZE> cache_aligned;

template<class C, typename boost::enable_if<boost::is_base_of<aligned_traits, C>, int>::type=0>
int get_alignment()
{
    return C::alignment;
}

template<class C, typename boost::disable_if<boost::is_base_of<aligned_traits, C>, int>::type=0>
int get_alignment()
{
    return 0;
}

#endif

