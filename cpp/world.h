#ifndef __WORLD
#define __WORLD

#include "common.h"
#include "geometry.h"
#include "random.h"
#include "agent.h"
#include "chooser.h"
#include "log_output.h"

#define PROPERTIES                                                      \
    _P(float, , , auto_immunity, 0)                                     \
    _P(float, , , travel_prob, 0)                                       \
    _P(float, , , infectiousness, 2.5)                                  \
    _P(float, , , appeal_factor, 0.5)                                   \
    _P(float, , , recovery_time, 12)                                    \
    _P(float, , , recovery_sd, 5)                                       \
    _P(float, , , gestating_time, 5)                                    \
    _P(float, , , gestating_sd, 2)                                      \
    _P(U32  , , , initial_infected, 100)                                \
    _P(U32, , ,   city_count, 0)                                        \
    _P(U32, , ,   min_city_count, 10)                                   \
    _P(U32, , ,   population, 10000)                                    \
    _P(U32, , ,   world_size, 100)                                      \
    _P(float, , , infected_cities, 0.5)                                 \
    _P(float, , , initial_infected_power, 0.4)                          \
    _P(float, , , travel, 0)                                            \
    _P(int  , , , thread_count, 0)                                      \
    _P(float, , , min_days, 0)                                          \
    _P(float, , , max_days, 0)                                          \
    _P(float, , , mobility_max, 0)                                      \
    _P(float, , , mobility_average, 0)                                  \
    _P(float, city, _, max_density, 5000)                               \
    _P(float, city, _, min_density, 1000)                               \
    _P(float, city, _, auto_power, 0.67)                                \
    _P(float, city, _, auto_divider, 250)                               \
    _P(float, city, _, max_pop, 0)                                      \
    _P(float, city, _, min_pop, 0)                                      \
    _P(float, city, _, min_count, 5)                                    \
    _P(float, city, _, min_size_multiplier, 2.5)                        \
    _P(float, city, _, pop_ratio_power, 0.5)                            \
    _P(float, city, _, exposure, 0.001)                                 \
    _P(float, city, _, appeal_power, 0.4)                               \
    _P(float, city, _, auto_max_pop, 0.2)                               \
    
class world
{
public:
    struct agent_info
    {
        U32 max_pop = 0;
        vector<city*> cities;
    };
private:
    vector<city*> my_cities;
    properties *my_props;
    log_output *my_logger;
    agent_manager my_agent_manager;
    vector<agent_info> cities_by_agent;
    U32 agent_max_pop = 0;
    random::lognormal gestation_generator;
    random::lognormal recovery_generator;
    float infection_prob;
    float mobility_threshold;
    float mobility_multiplier;
    day_number day = 0;
    U32 infected = 0;
    U32 total_infected = 0;
    U32 prev_infected = 0;
    U32 prev_total = 0;
    U32 immune = 0;
    U32 untouched_cities = 0;
    U32 verbosity = 1;
    chooser<city, U32> city_chooser;
    bool build_complete = false;
    ptime start_time;
    ptime build_complete_time;
    //
    // Cached properties
    //
#undef _P
#define _P(TYPE, PREFIX, DELIM, NAME, DFLT) TYPE PREFIX##DELIM##NAME;
    PROPERTIES
public:
    world(properties *props);
    void build();
    void run(log_output &logger);
    const vector<city*> get_cities() const { return my_cities; };
    U32 get_gestation_interval() const;
    U32 get_recovery_interval() const;
    point get_random_location() const;
    city *get_random_city() const;
    day_number get_day() const { return day; };
    float make_city_size(U32 population) const;
    float make_mobility();
    float get_city_pop_ratio(U32 population);
    float get_infection_prob() const { return infection_prob; };
    U32 get_verbosity() const { return verbosity; };
    properties *get_props() const { return my_props; };
    bool still_interesting() const;
    void build_agent(epidemia_agent *ag);
    bool end_of_day();
    const ptime &get_start_time() const { return start_time; };
    const ptime &get_build_complete_time() const { return build_complete_time; };
#undef _P
#define _P(TYPE, PREFIX, DELIM, NAME, DFLT)          \
    TYPE get_##PREFIX##DELIM##NAME() const      \
    { return PREFIX##DELIM##NAME; };
    PROPERTIES
private:
    void load_props();
    float get_one_prop(const string &prefix, const string &name, float dflt);
    void add_cities();
    void infect_cities();
    void make_infection_prob();
    void make_agents();
    void show_mobility_data();
    void show_cities();
    void finish_build();
    void assign_cities_to_agents();
};

#endif
