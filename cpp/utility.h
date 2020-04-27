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
    return 1 - ((1 - p1) * (1 - p2));
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
 * string functions
 ***********************************************************************/

vector<string> split(const string &str, const string &delims);
string join(const vector<string> &vec, const string &delim="");

#endif
