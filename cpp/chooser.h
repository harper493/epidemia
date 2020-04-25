#ifndef __CHOOSER
#define __CHOOSER

#include "common.h"
#include "random.h"
#include "utility.h"

/************************************************************************
 * Class to create a rapid random weighted lookup among a collection of
 * objects.
 *
 * Template parameters:
 *
 * C: class of objects in collection
 * K: the key type (generally int, float or some variant thereof, must
 *    support the usual arithmetic operations)
 ***********************************************************************/

template<class C, class K>
class chooser
{
public:
    typedef function<K(const C*)> key_fn_t;
private:
    vector<C*> targets;
    vector<K> keys;
    vector<float> boundaries;
public:
    /************************************************************************
     * create - given a collection of objects, and a key function
     * for extractng the weight, build the structures that will
     * allow efficient choice
     ***********************************************************************/
    
    template<class COLL, typename enable_if<boost::is_same<typename COLL::value_type, C>, int>::type=0>
    void create(COLL &coll, key_fn_t key_fn)
    {
        float total = 0;
        for (C &c : coll) {
            targets.push_back(&c);
            K key = key_fn(&c);
            keys.push_back(key);
            total += key;
        }
        boundaries.push_back(0);
        float sum = 0;
        for (K key : keys) {
            sum += key;
            boundaries.push_back(sum/total);
        }
        boundaries.back() = 1;  // avoid rounding error
    }

    /************************************************************************
     * choose - choose oneof the objects at random, taking into account their
     * weights.
     ***********************************************************************/
    
    C *choose() const
    {
        K k = random::get_random();
        return targets[binary_search(boundaries, k)];
    }

    /************************************************************************
     * reset - delete the dynamic memory
     ***********************************************************************/

    void reset()
    {
        targets.clear();
        keys.clear();
        boundaries.clear();
    }
};

#endif
