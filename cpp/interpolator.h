#ifndef __INTERPOLATOR
#define __INTERPOLATOR

#include "common.h"
#include "utility.h"

template<class C>
class interpolator
{
public:
    typedef pair<C,C> paircc;
private:
    vector<C> x;
    vector<C> y;
public:
    template<class COLL,
             typename enable_if<boost::is_same<typename COLL::value_type, std::pair<C,C>>, int>::type=0>
    interpolator(const COLL &coll)
    {
        x.reserve(coll.size());
        y.reserve(coll.size());
        for (const auto &c : coll) {
            if (x.size() > 0) {
                debug_assert(c.first > x.back());
            }
            x.push_back(c.first);
            y.push_back(c.second);
        }
    }
    C operator()(const C &value)
    {
        debug_assert(value<=x.back());
        size_t idx = binary_search(x, value);
        return y[idx] + ((value - x[idx]) / (x[idx+1]-x[idx])) * (y[idx+1] - y[idx]);
    }
};


#endif
