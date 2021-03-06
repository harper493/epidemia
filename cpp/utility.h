#ifndef __UTILITY
#define __UTILITY

#include "common.h"
#include <boost/algorithm/string.hpp>
#include <tgmath.h>

/************************************************************************
 * add_probability - combine two independent probabilities correctly
 ***********************************************************************/

inline float add_probability(float p1, float p2)
{
    float result =  1 - ((1 - p1) * (1 - p2));
    debug_assert(result<=1);
    return result;
}

/************************************************************************
 * binary_search - make a binary search in a suitable table, return the
 * index of the one found
 ***********************************************************************/

template<class COLL, class C>
size_t binary_search(const COLL &coll, const C &value)
{
    size_t lb = 0;
    size_t ub = coll.size();
    size_t mid;
    while (lb+1 < ub) {
        mid = (lb + ub)/2;
        C v = coll[mid];
        if (value < v) {
            ub = mid;
        } else {
            lb = mid;
        }
    }
    return lb;
}

/************************************************************************
 * copy - copy one container to another
 ***********************************************************************/

template<class COLL1, class COLL2,
         typename enable_if<boost::is_same<typename COLL1::value_type,
                                           typename COLL2::value_type>,int>::type=0>
inline void copy_container(const COLL1 &src, COLL2 &dst)
{
    std::copy(src.begin(), src.end(), std::back_inserter(dst));
}

/************************************************************************
 * make_rms - compute the RMS values of a collection
 ***********************************************************************/

template<class COLL>
typename COLL::value_type make_rms(const COLL &coll)
{
    typedef typename COLL::value_type value_type;
    value_type sumsq = 0;
    for (const value_type &c : coll) {
        sumsq += c * c;
    }
    return sqrt(sumsq / coll.size());
}

/************************************************************************
 * round_sig - round and integer to a given number of significant figures
 ***********************************************************************/

template<class C>
inline C round_sig(C value, U32 digits)
{
    C vv = value>0 ? value : -value;
    int log = log10(value);
    int p10 = pow(10, log - digits + 1);
    C result = (S64(value / p10)) * p10;
    return value>0 ? result : -result;
}

/************************************************************************
 * Compute standard deviation given sum and sumsq.
 ***********************************************************************/

inline float compute_standard_deviation(U32 count, float sum, double sumsq)
{
    return max(count ? sqrt(count * sumsq - sum * sum) / count : 0, 0);
}

/************************************************************************
 * contains - return true iff the given object is in the given container
 ***********************************************************************/

template<class COLL>
bool contains(const COLL &coll, const typename COLL::value_type &key)
{
    typedef typename COLL::value_type key_type;
    return std::find(coll.begin(), coll.end(), const_cast<key_type>(key)) != coll.end();
}

/************************************************************************
 * find_in_map - find a given key in a map, or return NULL. Assumes
 * that the map's data type is a pointer
 ***********************************************************************/

template<class M, typename enable_if<boost::is_pointer<typename M::mapped_type>, int>::type=0>
inline typename M::mapped_type find_in_map(const M &m, const typename M::key_type &k)
{
    auto i = m.find(k);
    return i==m.end() ? NULL : i->second;
}

/************************************************************************
 * string functions
 ***********************************************************************/

vector<string> split(const string &str, const string &delims);
string join(const vector<string> &vec, const string &delim="");
inline void join_to(string &first, const string &second, const string &delim)
{
    first = first.empty() ? second : first + delim + second;
}

vector<string> trim(const vector<string> &input);
void add_one_to_string(string &str);
void itoa(U32 value, char *ptr);

inline bool contains(const string &str, const string &key)
{
    return str.find(key) != string::npos;
}

/************************************************************************
 * system functions
 ***********************************************************************/

int get_system_core_count();

inline ptime ptime_now()
{
    return boost::posix_time::microsec_clock::local_time();
}

#endif
