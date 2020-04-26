#include "random.h"
#include <iostream>
#include <fstream>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <tgmath.h>

using std::ifstream;

/************************************************************************
 * static data
 ***********************************************************************/

boost::mt19937 random::rng;

interpolator<float> random::reciprocal::power_table(vector<pair<float,float>>{
    { 0.3484, 0.1 },
    { 0.3500, 0.2 },
    { 0.3521, 0.3 },
    { 0.3546, 0.4 },
    { 0.3578, 0.5 },
    { 0.3618, 0.6 },
    { 0.3670, 0.7 },
    { 0.3734, 0.8 },
    { 0.3817, 0.9 },
    { 0.3918, 1.0 },
    { 0.4038, 1.1 },
    { 0.4173, 1.2 },
    { 0.4318, 1.3 },
    { 0.4464, 1.4 },
    { 0.4609, 1.5 },
    { 0.4747, 1.6 },
    { 0.4879, 1.7 },
    { 0.5001, 1.8 },
    { 0.5116, 1.9 },
    { 0.5220, 2.0 },
    { 0.5317, 2.1 },
    { 0.5407, 2.2 },
    { 0.5489, 2.3 },
    { 0.5565, 2.4 },
    { 0.5636, 2.5 },
    { 0.5702, 2.6 },
    { 0.5762, 2.7 },
    { 0.5819, 2.8 },
    { 0.5870, 2.9 },
});

U32 random::true_random()
{
    U32 result = 0;
    ifstream rstr("/dev/urandom", ifstream::in|ifstream::binary);
    for (int i=0; i<4; ++i) {
        result <<= 8;
        char ch;
        rstr.get(ch);
        result |= (((U32)ch)&0xff);
    }
    return result;
}

void random::initialize(float seed)
{
    rng.seed(seed==0 ? true_random(): seed);
}

int random::uniform_int(int minimum, int maximum)
{
    boost::uniform_int<> ui(minimum, maximum);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > 
        vg(rng, ui);
    return vg(); 
}

float random::uniform_real(float minimum, float maximum)
{
    boost::uniform_real<float> ui(minimum, maximum);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<float> > 
        vg(rng, ui);
    return vg(); 
}

float random::get_random()
{
    boost::uniform_01<float> ui;
    boost::variate_generator<boost::mt19937&, boost::uniform_01<float> > 
        vg(rng, ui);
    return vg(); 
}

void random::lognormal::reset(float mean_, float sd_, float min/*=-1*/)
{
    offset = min>=0 ? min : mean_/2;
    float m = mean_ - offset;
    mean = mean_;
    sd = sd_;
    mu = log(m*m / sqrt(m*m + sd*sd));
    sigma = sqrt(log(1 + pow((sd / m), 2)));
    boost::random::lognormal_distribution<float>::param_type params(mu, sigma);
    dist.param(params);
}

float random::lognormal::operator()() const
{
    boost::variate_generator<boost::mt19937&, boost::random::lognormal_distribution<float> > vg(rng, dist);
    return vg() + offset;
}

void random::reciprocal::reset(float min_, float max_, U32 count, float mean)
{
    if (max_ < min_ * 2 || mean < min_ * 1.2 || mean > max_ * 0.8) {
        power = 1;              // bad inputs, give up
    } else {
        min_value = min_;
        max_value = max_;
        float mean_ratio = log(mean/min_) / log(max_/min_);
        power = power_table(mean_ratio);
    }
    max_x = 1 / pow(min_, 1/power);
    min_x = 1 / pow(max_, 1/power);
}

float random::reciprocal::operator()() const
{
    float r = uniform_real(min_x, max_x);
    return 1 / pow(r, power);
}


