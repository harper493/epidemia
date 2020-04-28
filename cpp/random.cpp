#include "random.h"
#include <iostream>
#include <fstream>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <tgmath.h>
#include <algorithm>
#include <numeric>

using std::ifstream;

/************************************************************************
 * static data
 ***********************************************************************/

boost::mt19937 random::rng;

interpolator<float> random::reciprocal::power_table(vector<pair<float,float>>{
    { 0.1601, 0.1 },
    { 0.1731, 0.2 },
    { 0.1882, 0.3 },
    { 0.2057, 0.4 },
    { 0.2262, 0.5 },
    { 0.2498, 0.6 },
    { 0.2776, 0.7 },
    { 0.3037, 0.8 },
    { 0.3312, 0.9 },
    { 0.3573, 1.0 },
    { 0.3811, 1.1 },
    { 0.4026, 1.2 },
    { 0.4230, 1.3 },
    { 0.4400, 1.4 },
    { 0.4551, 1.5 },
    { 0.4674, 1.6 },
    { 0.4806, 1.7 },
    { 0.4903, 1.8 },
    { 0.5011, 1.9 },
    { 0.5099, 2.0 },
    { 0.5179, 2.1 },
    { 0.5241, 2.2 },
    { 0.5307, 2.3 },
    { 0.5368, 2.4 },
    { 0.5424, 2.5 },
    { 0.5486, 2.6 },
    { 0.5523, 2.7 },
    { 0.5568, 2.8 },
    { 0.5609, 2.9 },
    { 1,      10 },
});

/************************************************************************
 * true_random - get a hardware sourced truly random number
 ***********************************************************************/

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

/************************************************************************
 * initialize - set the initial seed
 ***********************************************************************/

void random::initialize(float seed)
{
    rng.seed(seed==0 ? true_random(): seed);
}

/************************************************************************
 * uniform_int - generate aninteger in the given range
 ***********************************************************************/

int random::uniform_int(int minimum, int maximum)
{
    boost::uniform_int<> ui(minimum, maximum);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > 
        vg(rng, ui);
    return vg(); 
}

/************************************************************************
 * uniform_real - generate real in the range [minimum, maximum)
 ***********************************************************************/

float random::uniform_real(float minimum, float maximum)
{
    boost::uniform_real<float> ui(minimum, maximum);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<float> > 
        vg(rng, ui);
    return vg(); 
}

/************************************************************************
 * get_random - get a random real in the range [0,1)
 ***********************************************************************/

float random::get_random()
{
    boost::uniform_01<float> ui;
    boost::variate_generator<boost::mt19937&, boost::uniform_01<float> > 
        vg(rng, ui);
    return vg(); 
}

/************************************************************************
 * lognormal - generate numbers with lognormal distribution
 ***********************************************************************/

/************************************************************************
 * reset - set the parameters of the distribution oto match the given
 * mean and SD. If a minimum value is specified, this will be the
 * lower bound of the values. Otherwise, min/2 is used by default.
 ***********************************************************************/

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

/************************************************************************
 * operator() - get a number from the distribution
 ***********************************************************************/

float random::lognormal::operator()() const
{
    boost::variate_generator<boost::mt19937&, boost::random::lognormal_distribution<float> > vg(rng, dist);
    return vg() + offset;
}

/************************************************************************
 * reciprocal - generate numbers with a reciprocal-like distribution.
 *
 * More exactly, generate numbers that follow the distribution
 * y = 1 / x^p where x is a uniform random number in the appropriate
 * range, and p is a power chosen to make the mean come out right.
 * A larger value of p results in a more "axis hugging" distribution,
 * a smaller value less so.
 *
 * Thereis no straightforward mathematical approach to this. We use
 * a table which relates the power to the mean, whic is good enough.
 ***********************************************************************/

void random::reciprocal::reset(float min_, float max_, U32 count_, float mean_)
{
    min_value = min_;
    max_value = max_;
    count = count_;
    mean = mean_;
    if (max_ < min_ * 2 || mean < min_ * 1.2 || mean > max_ * 0.8) {
        power = 1;              // bad inputs, give up
    } else {
        float mean_ratio = log(mean/min_) / log(max_/min_);
        power = power_table(mean_ratio);
    }
    max_x = 1 / pow(min_, 1/power);
    min_x = 1 / pow(max_, 1/power);
}

/************************************************************************
 * operator() - get a number from the distribution
 ***********************************************************************/

float random::reciprocal::operator()() const
{
    float r = uniform_real(min_x, max_x);
    return 1 / pow(r, power);
}

/************************************************************************
 * get_values_int - return a vector of integers satisfying the parameters
 * of the distribution, ordered largest first.
 *
 * We first generate a random set, then tweak the results to make the
 * result exact.
 ***********************************************************************/

vector<U32> random::reciprocal::get_values_int() const
{
    vector<U32> result;
    size_t total = mean * count;
    result.reserve(count);
    for (size_t i=0; i<count; ++i) {
        result.push_back(this->operator()());
    }
    std::sort(result.begin(), result.end(), [](size_t a, size_t b){ return a>b; });
    result.front() = max_value;
    result.back() = min_value;
    replace_if(result.begin(), result.end(), [&](size_t sz){ return sz<min_value; }, min_value);
    size_t target_total = std::accumulate(result.begin(), result.end(), 0) - max_value - min_value;
    size_t center_pop = total - max_value - min_value;
    float ratio = ((float)(center_pop)) / ((float)target_total);
    for (size_t i=1; i<result.size()-1; ++i) {
        result[i] *= ratio;
    }
    size_t final_pop = std::accumulate(result.begin(), result.end(), 0);
    size_t delta = final_pop > total ? -1 : 1;
    for (size_t i=1; i<result.size() && final_pop!=total; ++i) {
        if (result[i] >= min_value - delta) {
            result[i] += delta;
            final_pop += delta;
        }
    }
    return result;
}


