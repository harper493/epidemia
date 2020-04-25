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
        float power = 1;
    public:
        reciprocal() { };
        reciprocal(float min_, float max_, U32 count, float mean)
        {
            reset(min_, max_, count, mean);
        }
        void reset(float min_, float max_, U32 count, float mean);
        float operator()() const;
        static interpolator<float> power_table;
    };
private:
    static boost::mt19937 rng;
public:
    static void initialize(float seed=0);
    static float get_random();
    static int uniform_int(int minimum, int maximum);
    static float uniform_real(float minimum, float maximum);
    static U32 true_random();
};

#endif
