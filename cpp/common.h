
#ifndef __COMMON
#define __COMMON

/************************************************************************
 * Basic common type definitions
 ***********************************************************************/

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <set>
#include <functional>
#include <iostream>
#include <fstream>
#include <tgmath.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/operators.hpp>
#include <boost/format.hpp>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/date_time.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/thread/condition.hpp>
#include <limits.h>
#include <typeinfo>

using std::vector;
using std::string;
using std::map;
using std::set;
using std::function;
using std::pair;
using std::make_pair;
using std::nothrow;
using std::array;
using boost::lexical_cast;
using boost::bind;
using std::unique_ptr;
using std::auto_ptr;
using std::make_pair;
using boost::is_same;
using boost::optional;
using boost::enable_if;
using boost::disable_if;
using boost::regex;
using boost::smatch;
using boost::mutex;
using boost::try_mutex;
using boost::recursive_mutex;
using boost::condition;
using boost::format;

namespace bintr = boost::intrusive;
namespace bfs = boost::filesystem;
namespace bpt = boost::posix_time;

using bpt::ptime;

typedef boost::multiprecision::uint256_t U256;
typedef __uint128_t U128;
typedef unsigned long long U64;
typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;

typedef boost::multiprecision::int256_t S256;
typedef __int128_t S128;
typedef signed long long S64;
typedef signed int S32;
typedef signed short S16;
typedef signed char S8;

typedef U32 day_number;

class person;
class city;
class world;
class cluster;
class properties;

/************************************************************************
 * Traits for widespread use
 ***********************************************************************/

struct streamable_traits { };

/************************************************************************
 * Global data
 ***********************************************************************/

extern world *the_world;

/************************************************************************
 * Compiler specific macros
 ***********************************************************************/

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)
#define _aligned(sz) __attribute__((aligned(sz)))
#define _packed __attribute__((packed))
#define _always_inline __attribute__((always_inline))
#define _flatten // __attribute__((flatten))
#define CACHE_LINE_SIZE 64
#define _cache_aligned __attribute__((aligned(CACHE_LINE_SIZE)))
#define MAKE_CACHE_ALIGNED(SIZE) ((SIZE + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1))
#define IS_CACHE_ALIGNED(ADDRESS) ((((U64)(ADDRESS)) & (CACHE_LINE_SIZE-1))==0)
#define CACHE_LINES_IN(SIZE) (((SIZE + CACHE_LINE_SIZE -1)/CACHE_LINE_SIZE))
#define CACHE_BARRIER(NAME) _cache_aligner NAME;
#define POINTERS_PER_CACHE_LINE (CACHE_LINE_SIZE / sizeof(void*))
#define MEMORY_BARRIER __asm__ __volatile__("":::"memory")

/************************************************************************
 * Widely used function types
 ***********************************************************************/

/************************************************************************
 * debug_assert macro
 ***********************************************************************/

#ifdef DEBUG
#define debug_assert(PRED) assert(PRED)
#else
#define debug_assert(PRED)
#endif

/************************************************************************
 * Generic streaming operators
 ***********************************************************************/

template<class C,
         class OSTR,
         typename enable_if<boost::is_base_of<streamable_traits, C>, int>::type=0,
         typename enable_if<boost::is_base_of<std::ostream, OSTR>, int>::type=0>
OSTR &operator<<(OSTR &ostr, const C &c)
{
    ostr << c.str();
    return ostr;
}

template<class C,
         class ISTR,
         typename enable_if<boost::is_base_of<streamable_traits, C>, int>::type=0,
         typename enable_if<boost::is_base_of<std::istream, ISTR>, int>::type=0>
ISTR &operator>>(ISTR &istr, C &c)
{
    string s;
    istr >> s;
    c.parse(s);
    return istr;
}

/************************************************************************
 * Compiler specific macros
 ***********************************************************************/

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)
#define _aligned(sz) __attribute__((aligned(sz)))
#define _packed __attribute__((packed))
#define _always_inline __attribute__((always_inline))
#define _flatten // __attribute__((flatten))
#define CACHE_LINE_SIZE 64
#define _cache_aligned __attribute__((aligned(CACHE_LINE_SIZE)))
#define MAKE_CACHE_ALIGNED(SIZE) ((SIZE + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1))
#define IS_CACHE_ALIGNED(ADDRESS) ((((U64)(ADDRESS)) & (CACHE_LINE_SIZE-1))==0)
#define CACHE_LINES_IN(SIZE) (((SIZE + CACHE_LINE_SIZE -1)/CACHE_LINE_SIZE))
struct _cache_aligner { } _cache_aligned;
#define CACHE_BARRIER(NAME) _cache_aligner NAME;
#define POINTERS_PER_CACHE_LINE (CACHE_LINE_SIZE / sizeof(void*))
#define MEMORY_BARRIER __asm__ __volatile__("":::"memory")

/************************************************************************
 * Civilized versions of min and max that will compare any two types
 * that can be compared. Note the conditional trick to deduce the
 * most appropriate return type: http://www.artima.com/cppsource/foreach.html
 ***********************************************************************/

template<class T1, class T2>
decltype(true?T1():T2()) min(T1 t1, T2 t2) 
{
    if (t1<t2) {
        return t1;
    } else {
        return t2;
    }
}

template<class T1, class T2>
decltype(true?T1():T2()) max(T1 t1, T2 t2) 
{
    if (t1>t2)  {
        return t1;
    } else {
        return t2;
    }
}

template<class T1, class T2, typename... ARGS>
decltype(true?T1():T2()) min(T1 t1, T2 t2, ARGS ...args) 
{
    return min(min(t1, t2), args...);
}

template<class T1, class T2, typename... ARGS>
decltype(true?T1():T2()) max(T1 t1, T2 t2, ARGS ...args) 
{
    return max(max(t1, t2), args...);
}


#endif
