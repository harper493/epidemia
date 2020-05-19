#include "world.h"
#include "city.h"
#include "cluster.h"
#include "utility.h"
#include "formatted.h"
#include "chooser.h"
#include "properties.h"
#include "interpolator.h"
#include "command.h"
#include <tgmath.h>
#include <algorithm>
#include <numeric>
#include <boost/range/adaptors.hpp>

/************************************************************************
 * constructor
 ***********************************************************************/

world::world(properties *props)
    : my_props(props)
{
    load_props();
}

/************************************************************************
 * load_props - load static parameters from properties file. We load
 * each value and also create a corresponding base.... version that we
 * can use when we adjust parameters.
 ***********************************************************************/

float world::get_one_prop(const string &prefix, const string &name, float dflt)
{
    if (prefix.empty()) {
        return my_props->get_numeric(name, dflt);
    } else {
        return my_props->get_numeric(vector<string>{prefix, name}, dflt);
    }
}

void world::load_props()
{
#undef _P
#define _P(TYPE, PREFIX, DELIM, NAME, DFLT)                     \
    PREFIX##DELIM##NAME = get_one_prop(#PREFIX, #NAME, DFLT);
    
    PROPERTIES;
#undef _P
#define _P(TYPE, PREFIX, DELIM, NAME, DFLT)                             \
    if (string(#PREFIX).empty()) {                                      \
        base_##PREFIX##DELIM##NAME =                                    \
            my_props->get_numeric(vector<string>{"base", #NAME}, PREFIX##DELIM##NAME); \
    } else {                                                            \
        base_##PREFIX##DELIM##NAME =                                    \
            my_props->get_numeric(vector<string>{"base", #PREFIX, #NAME}, PREFIX##DELIM##NAME); \
    }
    
    PROPERTIES;
    
    for (const auto *p : *my_props) {
        if (!boost::starts_with(p->get_name(), "base.")) {
            string base_name = string("base.") + p->get_name();
            if (my_props->get(base_name).empty()) {
                my_props->add_property(base_name, p->get_value());
            }
        }
    }
    gestation_generator.reset(gestating_time, gestating_sd);
    recovery_generator.reset(recovery_time, recovery_sd);
}

/************************************************************************
 * build - create the world
 ***********************************************************************/

void world::build()
{
    start_time = ptime_now();
    adjust_distance();
    cluster_type::build(this);
    mobility_threshold = 3 * mobility_average / mobility_max;
    mobility_multiplier = pow(mobility_max, 3) / (9 * pow(mobility_average, 2));
    add_cities();
    assign_cities_to_agents();
    make_agents();
}

void world::finish_build()
{
    if (!build_complete) {
        build_complete = true;
        make_infection_prob();
        for (city *c : my_cities) {
            c->finalize();
        }
        infect_cities();
        for (auto i : cluster_type::get_cluster_types()) {
            cluster_type *ct = i.second;
            ct->finalize(this);
        }
    }
    build_complete_time = ptime_now();
}

/************************************************************************
 * add_cities - calculate the populations of the cities, and add them
 ***********************************************************************/

void world::add_cities()
{
    //
    // Figure out a good value for the number of cities, if this
    // was not in the spec, and the min and max populations
    //
    if (city_count==0 || city_max_pop==0) {
        if (city_count==0) {
            city_count = max(pow(population, city_auto_power) / city_auto_divider,
                             min_city_count);
        }
        city_max_pop = round_sig(population * city_auto_max_pop, 2);
        city_min_pop = round_sig((population - city_max_pop) /
                                 (city_count * city_min_size_multiplier), 2);
    }
    //
    // Now generate city populations that fit the criteria.
    //
    random::reciprocal cpr(city_min_pop, city_max_pop, city_count, population/city_count);
    vector<U32> city_pops = cpr.get_values_int();
    //
    // Finally, create the cities
    //
    for (size_t i=0; i<city_count; ++i) {    
        string cname = formatted("C%d", i+1);
        U32 pop = city_pops[i];
        float this_size = make_city_size(pop);
        point loc = get_random_location();
        bool good = false;
        while (!good) {
            good = true;
            loc = get_random_location();
            for (city *other : my_cities) {
                if (loc.distance(other->get_location()) + other->get_size() + this_size < city_min_distance) {
                    good = false;
                    break;
                }
            }
        }
        city *c = new city(cname, this, pop, loc);
        my_cities.push_back(c);
    }
    //
    // populate the population-based chooser
    //
    city_chooser.create(my_cities, &city::get_target_pop);
}

/************************************************************************
 * assign_cities_to_agents - figure out which agents will deal with which
 * cities, trying to keep the population about equally spread.
 *
 * We start with the biggest cities, which may need to be divided amongst
 * multiple agents. Then we combine big cities. Whenever we can't fit
 * the next big city, we take the smallest cities in inverse size
 * order as "stocking fillers". When every agent has reached their
 * target population, if there are any (small) cities left, we just assign
 * them round robin to the agents.
 ***********************************************************************/

void world::assign_cities_to_agents()
{
    if (thread_count==0) {
        thread_count = get_system_core_count() * 2;
    }
    thread_count = max(1, thread_count);
    if (thread_count > 1) {
        agent_max_pop = population / thread_count;
        size_t big_cities = 0;
        size_t small_cities = my_cities.size() - 1;
        U32 remaining_population = (my_cities[big_cities])->get_target_pop();
        for (size_t a=0; a<thread_count; ++a) {
            if (big_cities >= small_cities) {
                thread_count = a;
                break;
            }
            cities_by_agent.emplace_back();
            agent_info &this_ai = cities_by_agent.back();
            U32 remaining_space = agent_max_pop;
            this_ai.cities.push_back(my_cities[big_cities]);
            if (remaining_population >= remaining_space) {
                //
                // This city still has enough population to fill
                // an entire agent
                //
                if (remaining_population==remaining_space) {
                    ++big_cities;
                }
                remaining_population -= agent_max_pop;
                remaining_space = 0;
                this_ai.max_pop = agent_max_pop;
            } else {
                this_ai.max_pop = remaining_population;
                remaining_space -= remaining_population;
                //
                // We have some left over capacity after the first big city
                //
                do {
                    if (big_cities>=small_cities) {
                        break;
                    }
                    ++big_cities;
                    remaining_population = (my_cities[big_cities])->get_target_pop();
                    if (remaining_space >= remaining_population) {
                        //
                        // We can fit this big city into the remaining space
                        //
                        this_ai.cities.push_back(my_cities[big_cities]);
                        remaining_space -= remaining_population;
                    } else {
                        //
                        // Out of space for the big guys. Pack in small cities
                        // until we run out of space
                        //
                        while (big_cities < small_cities
                               && (my_cities[small_cities])->get_target_pop() <= remaining_space) {
                            this_ai.cities.push_back(my_cities[small_cities]);
                            remaining_space -= (my_cities[small_cities])->get_target_pop();
                            --small_cities;
                        }
                        break;
                    }
                } while (true);
            }
        }
        //
        // We may still have some cities left. Just assign them to agents
        // one at a time, starting with the later ones that will generally
        // mostly have small guys.
        //
        while (big_cities <= small_cities) {
            for (auto &ai : cities_by_agent | boost::adaptors::reversed) {
                ai.cities.push_back(my_cities[small_cities]);
                --small_cities;
                if (small_cities<=big_cities) {
                    break;
                }
            }
        }
    }
}

/************************************************************************
 * infect_cities - create the initial number of infections
 ***********************************************************************/

void world::infect_cities()
{
    //
    // first decide how many people will be infected
    //
    if (initial_infected==0) {
        initial_infected = round_sig(pow(population, initial_infected_power), 1);
    } else {
        initial_infected = min(initial_infected, round_sig(sqrt(population), 1));
    }
    //
    // now decide how many cities will be infected
    //
    U32 city_count = 0;
    if (infected_cities==0) {
        city_count = my_cities.size();
    } else if (infected_cities>1) {
        city_count = infected_cities;
    } else {
        city_count = my_cities.size() * infected_cities;
    }
    //
    // now decide which ones will be infected
    //
    vector<city*> infectees;
    copy_container(my_cities, infectees);
    if (city_count<my_cities.size()) {
        std::random_shuffle(infectees.begin(), infectees.end());
        infectees.resize(city_count);
    }
    //
    // make sure each has at least one infected
    //
    for (city *c : infectees) {
        while (true) {
            person *p = c->get_random_person();
            if (p->is_susceptible()) {
                p->force_infect(0);
                break;
            }
        }
    }
    //
    // now randomly infect others, biassed by city size. The while loops
    // are to make sure we don't try to infect too many in one city,
    // and that we don't re-infect the same person
    //
    chooser<city, U32> city_chooser(infectees, &city::get_population);
    for (size_t i=0; i<(initial_infected - city_count); ++i) {
        while (true) {
            city *target = city_chooser.choose();
            if (target->get_infected() < target->get_population() / 2) {
                while (true) {
                    person *victim = target->get_random_person();
                    if (victim->is_susceptible()) {
                        victim->force_infect(0);
                    }
                    break;
                }
                break;
            }
        }
    }
}

/************************************************************************
 * make_infection_prob - calculate the contribution of a single
 * infectee, based on average cluster sizes
 ***********************************************************************/

#define P std::pair<float,float>

void world::make_infection_prob()
{
    static vector<pair<float,float>> immunity_correction{
        P( 0, 0.32 ),
        P( 0.25, 0.44 ),
        P( 0.5, 0.67 ),
        P( 0.75, 1.36 ),
        P( 0.9, 4 )
    };
    static interpolator<float> immunity_corrector(immunity_correction);
    float inf = min(0.9, infectiousness);
    float correction = immunity_corrector(base_auto_immunity);
    U32 exposure_time = base_recovery_time - base_gestating_time;
    float cluster_factor = my_props->get_numeric("cluster_factor");
    if (cluster_factor==0) {
        for (auto iter : cluster_type::get_cluster_types()) {
            cluster_type *ct = iter.second;
            cluster_factor += (ct->base_size_rms - 1) * pow(ct->base_influence, 2);
        }
    }
    infection_prob = ((float)infectiousness) * correction / (exposure_time * cluster_factor);
}

/************************************************************************
 * make_agents - create the appropriate number of agents
 ***********************************************************************/

void world::make_agents()
{
    my_agent_manager.build(thread_count,
                           bind(&epidemia_agent::factory, _1, _2, _3, this));
}

/************************************************************************
 * build_agent - tell an agent what to do once it has been created
 ***********************************************************************/

void world::build_agent(epidemia_agent *ag)
{
    agent_info &ai = cities_by_agent[ag->get_index()];
    if (thread_count>1) {
        ag->add_cities(ai.cities, ai.max_pop);
    } else {
        ag->add_cities(my_cities, 0);
    }
}

/************************************************************************
 * run - run until no longer interesting
 ***********************************************************************/

void world::run(log_output &logger)
{
    my_logger = &logger;
    epidemia_task task(this);
    my_agent_manager.execute(&task);
    my_agent_manager.terminate();
    if (the_args->get_verbosity()) {
        std::cout << task.show_timers() << std::endl;
    }
}

/************************************************************************
 * end_of_day - called via agent manager at the end of each day. Return true
 * to keep going, false to stop.
 ***********************************************************************/

bool world::end_of_day()
{
    if (day==0) {
        finish_build();
    }
    ++day;
    prev_infected = infected;
    prev_total = total_infected;
    infected = 0;
    total_infected = 0;
    immune = 0;
    untouched_cities = 0;
    vaccinated = 0;
    dead = 0;
    for (city *c : my_cities) {
        infected += c->get_infected();
        total_infected += c->get_total_infected();
        immune += c->get_immune();
        vaccinated += c->get_vaccinated();
        dead += c->get_dead();
        if (c->is_untouched()) {
            ++untouched_cities;
        }
    }
    max_infected = max(max_infected, infected);
    float growth = 100 * ((prev_total ? ((float)total_infected) / prev_total: 1) - 1);
    my_logger->put_line(day, 0, infected, total_infected, growth, immune, vaccinated, dead, untouched_cities);
    if (the_args->get_log_cities()) {
        for (city *c : my_cities) {
            my_logger->put_line(day, c->index, c->infected,
                                c->total_infected, 0, c->immune,
                                c->vaccinated, c->dead, c->is_untouched()?1:0);
        }
    }
    my_logger->flush();
    return still_interesting();
}

/************************************************************************
 * still_interesting - evaluate whether we should continue
 ***********************************************************************/

bool world::still_interesting() const
{
    bool result = true;
    if (min_days > 0 && day < min_days) {
        result = true;
    } else if (max_days > 0 && day > max_days) {
        result = false;
    } else {
        result = (max_infected > initial_infected * 10)
                  ? infected > initial_infected
                  : infected * 5 > initial_infected;
    }
    return result;
}

/************************************************************************
 * get_gestation_interval - get a randomly generated gestation interval
 * according to the parameters
 ***********************************************************************/

U32 world::get_gestation_interval() const
{
    return gestation_generator();
}

/************************************************************************
 * get_recovery_interval - get a randomly generated recovery interval
 * according to the parameters
 ***********************************************************************/

U32 world::get_recovery_interval() const
{
    return recovery_generator();
}

/************************************************************************
 * make_city_size - given the population of a city, calculate its
 * size
 ***********************************************************************/

float world::make_city_size(U32 pop) const
{
    float pop_ratio = ((float)(pop)) / city_max_pop;
    //float density = sqrt(pop_ratio) * (city_max_density - city_min_density) + city_min_density;
    float density = city_min_density + (pop - city_min_pop) *
        ((float)(city_max_density - city_min_density)) / (city_max_pop - city_min_pop);
    float area = pop / density;
    float radius = sqrt(area/3.14);
    return radius;
}

/************************************************************************
 * adjust_distance - apply the distance parameter. We adjust every
 * parameter that has a "distance_min_..." counterpart, in the ratio
 * from min to normal according to the distance parameter.
 ***********************************************************************/

void world::adjust_distance()
{
    static const string prefix("distance.min.");
    if (distance > 0) {
        for (const auto *d : *my_props) {
            if (boost::starts_with(d->get_name(), prefix)) {
                string prop_name = d->get_name().substr(prefix.size());
                float v = my_props->get_numeric(prop_name);
                float min_v = my_props->get_numeric(d->get_name());
                float new_value = (v - min_v) * (1 - distance) + min_v;
                my_props->add_property(prop_name, lexical_cast<string>(new_value));
            }
        }
    }
}

/************************************************************************
 * get_city_pop_ratio - get the ratio to use for calculating the
 * per-person exposure for a city
 ***********************************************************************/

float world::get_city_pop_ratio(U32 population)
{
    float pop_ratio = ((float)population) / city_min_pop;
    return 1 / pow(pop_ratio, city_pop_ratio_power);
}

/************************************************************************
 * get_random_location - return a random location within the world
 ***********************************************************************/

point world::get_random_location() const
{
    return point(random::uniform_real(1, world_size-1), random::uniform_real(1, world_size-1));
}

/************************************************************************
 * get_random_city - get a random city, weighted by size
 ***********************************************************************/

city *world::get_random_city() const
{
    return city_chooser.choose();
}

/************************************************************************
 * make_mobility - generate a random mobility figure for a person,
 * based on the configured average and maximum values
 ***********************************************************************/

float world::make_mobility()
{
    float result = 0;
    float r = random::get_random();
    if (r < mobility_threshold) {
        result = r * r * mobility_multiplier;
    }
    return result;
}

/************************************************************************
 * show_mobility_data - analyse mobility info for the whole
 * population
 ***********************************************************************/

void world::show_mobility_data()
{
    float mob_sum = 0;
    double mob_sumsq = 0;
    float mob_max = 0;
    U32 mobs = 0;
    for (city *c : my_cities) {
        for (person *p : c->get_people()) {
            float m = p->get_mobility();
            if (m>0) {
                mobs += 1;
                mob_sum += m;
                mob_sumsq += m*m;
                mob_max = max(mob_max, m);
            }
        }
    }
    float mean = mob_sum / mobs;
    float sd = compute_standard_deviation(mobs, mob_sum, mob_sumsq);
    float overall_mean = mob_sum / population;
    std::cout << formatted("mobility intended mean %f max %f count %d mean %f sd %f "
                           "overall mean %f max %f thresh %f mult %f\n",
                           mobility_average, mobility_max,
                           mobs, mean, sd, overall_mean, mob_max, mobility_threshold, mobility_multiplier);
}

/************************************************************************
 * show_cities - show city information
 ***********************************************************************/

void world::show_cities(log_output &logger)
{
    for (city *c : my_cities) {
        c->log(logger);
    }
}
