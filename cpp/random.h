#ifndef __RANDOM
#define __RANDOM

#include "common.h"
#include "interpolator.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/lognormal_distribution.hpp>

class random
{
public:
    class lognormal
    {
    private:
        float mu;
        float sigma;
        float mean;
        float sd;
        float offset;
        boost::random::lognormal_distribution<float> dist;
    public:
        void reset(float mean_, float sd_, float min=-1);
        float operator()() const;
    };
    class reciprocal
    {
    private:
        float min_value = 0;
        float max_value = 0;
        float min_x = 0;
        float max_x = 0;
        U32 count = 0;
        float mean = 0;
        float power = 1;
    public:
        reciprocal() { };
        reciprocal(float min_, float max_, U32 count_, float mean_)
        {
            reset(min_, max_, count_, mean_);
        }
        void reset(float min_, float max_, U32 count_, float mean_);
        float operator()() const;
        vector<U32> get_values_int() const;
    private:
        static interpolator<float> power_table;
    };
private:
    static __thread boost::mt19937 *generator;
    static U32 my_seed;
    static mutex my_lock;
public:
    static void initialize(U32 seed=0);
    static float get_random();
    static int uniform_int(int minimum, int maximum);
    static float uniform_real(float minimum, float maximum);
    static U32 true_random();
    template<class C>
    static const typename C::value_type &uniform_choice(const C &coll)
    {
        return coll[uniform_int(0, coll.size()-1)];
    }
private:
    static void init_thread();
};

#endif
