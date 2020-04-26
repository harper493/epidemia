#ifndef __WORLD
#define __WORLD

#include "common.h"
#include "geometry.h"
#include "random.h"

#define PROPERTIES                                                      \
    _P(float, , , auto_immunity, 0)                                     \
    _P(float, , , travel_prob, 0)                                       \
    _P(float, , , infectiousness, 2.5)                                  \
    _P(float, , , appeal_factor, 0.5)                                   \
    _P(float, , , recovery_time, 12)                                    \
    _P(float, , , recovery_sd, 5)                                       \
    _P(float, , , gestating_time, 5)                                    \
    _P(float, , , gestating_sd, 2)                                      \
    _P(float, , , initial_infected, 100)                                \
    _P(U32, , ,   city_count, 0)                                        \
    _P(U32, , ,   min_city_count, 10)                                   \
    _P(U32, , ,   population, 10000)                                    \
    _P(U32, , ,   world_size, 100)                                      \
    _P(float, , , infected_cities, 0.5)                                 \
    _P(float, , , travel, 0)                                            \
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
    _P(float, city, _, auto_max_pop, 0.3)                                 \
    
class world
{
private:
    vector<city*> my_cities;
    properties *my_props;
    random::lognormal gestation_generator;
    random::lognormal recovery_generator;
    float infection_prob;
    //
    // Cached properties
    //
#undef _P
#define _P(TYPE, PREFIX, DELIM, NAME, DFLT) TYPE PREFIX##DELIM##NAME;
    PROPERTIES
public:
    world(properties *props);
    void build();
    const vector<city*> get_cities() const { return my_cities; };
    U32 get_gestation_interval() const;
    U32 get_recovery_interval() const;
    point get_random_location() const;
    U32 make_city_size(U32 population) const;
    float get_city_pop_ratio(U32 population);
    float get_infection_prob() const { return infection_prob; };
    properties *get_props() const { return my_props; };
#undef _P
#define _P(TYPE, PREFIX, DELIM, NAME, DFLT)          \
    TYPE get_##PREFIX##DELIM##NAME() const      \
    { return PREFIX##DELIM##NAME; };
    PROPERTIES
private:
    void load_props();
    float get_one_prop(const string &prefix, const string &name, float dflt);
    void add_cities();
    void make_infection_prob();
};

#endif
