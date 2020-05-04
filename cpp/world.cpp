#include "world.h"
#include "city.h"
#include "cluster.h"
#include "utility.h"
#include "formatted.h"
#include "chooser.h"
#include "properties.h"
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
    start_time = ptime_now();
    cluster_type::build(this);
    mobility_threshold = 3 * mobility_average / mobility_max;
    mobility_multiplier = pow(mobility_max, 3) / (9 * pow(mobility_average, 2));
    add_cities();
    if (the_args->get_verbosity()) {
        show_cities();
    }
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
        // show_cities();
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
        city *c = new city(cname, this, city_pops[i], get_random_location());
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
 * target population, if there are any (small) cities left, we just assihgn
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
    float cluster_factor = my_props->get_numeric("cluster_factor");
    if (cluster_factor==0) {
        for (auto iter : cluster_type::get_cluster_types()) {
            cluster_type *ct = iter.second;
            cluster_factor += ct->size_rms * pow(ct->influence, 2);
        }
    }
    infection_prob = ((float)infectiousness) / (exposure_time * cluster_factor);
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
    float growth = 100 * ((prev_total ? ((float)total_infected) / prev_total: 1) - 1);
    my_logger->put_line(day, 0, infected, total_infected, growth, immune, untouched_cities);
    prev_infected = infected;
    prev_total = total_infected;
    infected = 0;
    total_infected = 0;
    immune = 0;
    untouched_cities = 0;
    for (city *c : my_cities) {
        infected += c->get_infected();
        total_infected += c->get_total_infected();
        immune += c->get_immune();
        if (c->is_untouched()) {
            ++untouched_cities;
        }
    }
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
 * based on the configured average andmaximum values
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

void world::show_cities()
{
    for (city *c : my_cities) {
        std::cout << c->show() << std::endl;
    }
}
