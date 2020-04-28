#include "world.h"
#include "city.h"
#include "cluster.h"
#include "utility.h"
#include "formatted.h"
#include "chooser.h"
#include "properties.h"
#include <tgmath.h>
#include <algorithm>
#include <numeric>

/************************************************************************
 * constructor
 ***********************************************************************/

world::world(properties *props)
    : my_props(props)
{
    load_props();
}

/************************************************************************
 * load_props - load static parameters from properties file
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
#define _P(TYPE, PREFIX, DELIM, NAME, DFLT) PREFIX##DELIM##NAME = get_one_prop(#PREFIX, #NAME, DFLT);
    PROPERTIES
    gestation_generator.reset(gestating_time, gestating_sd);
    recovery_generator.reset(recovery_time, recovery_sd);
}

/************************************************************************
 * build - create the world
 ***********************************************************************/

void world::build()
{
    add_cities();
    for (city *c : my_cities) {
        c->add_people();
    }
    make_infection_prob();
    infect_cities();
    for (city *c : my_cities) {
        c->finalize();
    }
    make_agents();
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
        string cname = formatted("C%d", i);
        city *c = new city(cname, this, city_pops[i], get_random_location());
        my_cities.push_back(c);
        c->build_clusters();
        c->add_people();
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
        std::random_shuffle(infectees.begin()+1, infectees.end());
        infectees.resize(city_count);
    }
    //
    // make sure each has at least one infected
    //
    for (city *c : infectees) {
        c->get_random_person()->force_infect(0);
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

void world::make_infection_prob()
{
    U32 exposure_time = recovery_time - gestating_time;
    float cluster_factor = 0;
    for (auto iter : cluster_type::get_cluster_types()) {
        cluster_type *ct = iter.second;
        cluster_factor += ct->size_rms * pow(ct->influence, 2);        
    }
    infection_prob = ((float)infectiousness) / (exposure_time * cluster_factor);
}

/************************************************************************
 * make_agents - create the appropriate number of agents
 ***********************************************************************/

void world::make_agents()
{
    size_t agent_count = thread_count;
    if (thread_count==0) {
        thread_count = 1;
    }
    for (size_t i=0; i<thread_count; ++i) {
        string name = formatted("agent-%d", i);
        agent *a = new agent(name, (agent_count>1));
        a->run();
        my_agents.push_back(a);
    }
    for (city *c : my_cities) {
        my_agents[0]->add_city(c);
    }
}

/************************************************************************
 * run - run until no longer interesting
 ***********************************************************************/

void world::run()
{
    do {
        ++day;
        std::cout << formatted("Day %3d infected %6d total %6d growth %8.2f%% immune %6d\n",
                               day, infected, total_infected,
                               100*((((float)total_infected)/(prev_total+0.001))-1),
                               immune);
        for (agent *a : my_agents) {
            a->one_day_first(day);
        }
        for (agent *a : my_agents) {
            a->one_day_second(day);
        }
        prev_infected = infected;
        prev_total = total_infected;
        infected = 0;
        total_infected = 0;
        immune = 0;
        for (city *c : my_cities) {
            infected += c->get_infected();
            total_infected += c->get_total_infected();
            immune += c->get_immune();
        }
    } while (worth_continuing());
}

/************************************************************************
 * worth_continuing - evaluate whether we should continue
 ***********************************************************************/

bool world::worth_continuing() const
{
    bool result = true;
    if (min_days > 0 && day < min_days) {
        result = true;
    } else if (max_days > 0 && day > max_days) {
        result = false;
    } else {
        result = infected>0
            && ((infected > prev_infected)
                || (infected > population / 1000)
                || (total_infected + immune < population / 4));
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

U32 world::make_city_size(U32 pop) const
{
    float pop_ratio = (pop - city_min_pop) / (city_max_pop - city_min_pop);
    float density = pop_ratio * (city_max_density - city_min_density) + city_min_density;
    float area = pop / density;
    float radius = sqrt(area/3.14);
    return radius;
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
    return point(random::uniform_int(1, world_size-1), random::uniform_int(1, world_size-1));
}
