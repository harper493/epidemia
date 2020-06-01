#include "random.h"
#include "formatted.h"
#include "unit_test.h"
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

__thread boost::mt19937 *random::generator = NULL;
U32 random::my_seed = 0;
mutex random::my_lock;

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

UNIT_TEST_CLASS(random)

/************************************************************************
 * true_random - get a hardware sourced truly random number
 ***********************************************************************/

U32 random::true_random()
{
    lock_guard<mutex> lg(my_lock);
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

void random::initialize(U32 seed)
{
    my_seed = seed;
}

/************************************************************************
 * uniform_int - generate aninteger in the given range
 ***********************************************************************/

int random::uniform_int(int minimum, int maximum)
{
    init_thread();
    boost::uniform_int<> ui(minimum, maximum);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > 
        vg(*generator, ui);
    return vg(); 
}

/************************************************************************
 * uniform_real - generate real in the range [minimum, maximum)
 ***********************************************************************/

float random::uniform_real(float minimum, float maximum)
{
    init_thread();
    boost::uniform_real<float> ui(minimum, maximum);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<float> > 
        vg(*generator, ui);
    return vg(); 
}

/************************************************************************
 * get_random - get a random real in the range [0,1)
 ***********************************************************************/

float random::get_random()
{
    init_thread();
    boost::uniform_01<float> ui;
    boost::variate_generator<boost::mt19937&, boost::uniform_01<float> > 
        vg(*generator, ui);
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
    init_thread();
    boost::variate_generator<boost::mt19937&, boost::random::lognormal_distribution<float> >
        vg(*generator, dist);
    return vg() + offset;
}

/************************************************************************
 * reciprocal - generate numbers with the requested ditribution,
 * such that the min, max, mean and count are as requested.
 *
 * The name reflects an earlier approach to the problem.
 ***********************************************************************/

void random::reciprocal::reset(float min_, float max_, U32 count_, float mean_)
{
    min_value = min_;
    max_value = max_;
    count = count_;
    mean = mean_;
    power = (max_value - min_value) / (mean - min_value) - 1;
}

/************************************************************************
 * operator() - get a number from the distribution
 ***********************************************************************/

float random::reciprocal::operator()() const
{
    return min_value + (max_value - min_value) * pow(get_random(), power);
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


/************************************************************************
 * init_thread - check that the generator for this thread has been
 * initialised. Note that 'generator' is thread local, hence there are
 * no concurrency issues here.
 ***********************************************************************/

void random::init_thread()
{
    if (generator==NULL) {
        generator = new boost::mt19937();
        generator->seed(my_seed ? my_seed : true_random());
    }
}

/************************************************************************
 * unit test
 ***********************************************************************/

void _random_ut_1(float min_, float max_, float count, float mean_)
{
    random::reciprocal recip(min_, max_, count, mean_);
    auto values = recip.get_values_int();
    U32 sum = 0;
    for (U32 v : values) {
        sum += v;
    }
    float avg = ((float)sum) / count;
    UT_EQUAL(count, values.size());
    UT_ASSERT(avg+1 >= mean_ && avg-1 <= mean_, "mean should be %.0f, was %.2f", mean_, avg);
}

void random::unit_test()
{
    _random_ut_1(0, 10, 100, 5);
    _random_ut_1(10, 20, 100, 15);
    _random_ut_1(0, 10, 100, 1);
    _random_ut_1(0, 10, 100, 9);
    _random_ut_1(0, 10, 100, 3);
    _random_ut_1(0, 1000, 100, 300);
}

