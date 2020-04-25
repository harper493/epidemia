#ifndef __WORLD
#define __WORLD

#include "common.h"
#include "geometry.h"
#include "random.h"

#define PROPERTIES                         \
    _P(float, , , auto_immunity)             \
    _P(float, , , travel_prob)               \
    _P(float, , , infectiousness)            \
    _P(float, , , appeal_factor)             \
    _P(float, , , recovery_time)             \
    _P(float, , , recovery_sd)               \
    _P(float, , , gestating_time)            \
    _P(float, , , gestating_sd)              \
    _P(float, , , initial_infected)          \
    _P(U32, , ,   city_count)                \
    _P(U32, , ,   population)                \
    _P(U32, , ,   world_size)                \
    _P(float, , , infected_cities)           \
    _P(float, , , travel)                    \
    _P(float, city, _, max_density)         \
    _P(float, city, _, min_density)         \
    _P(float, city, _, auto_power)          \
    _P(float, city, _, auto_divider)        \
    _P(float, city, _, max_pop)             \
    _P(float, city, _, min_pop)             \
    _P(float, city, _, min_count)           \
    _P(float, city, _, min_size_multiplier) \
    _P(float, city, _, pop_ratio_power)     \
    _P(float, city, _, exposure)            \
    _P(float, city, _, appeal_power)        \
    _P(float, city, _, auto_max_pop)        \
    
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
#define _P(TYPE, PREFIX, DELIM, NAME) TYPE PREFIX##DELIM##NAME;
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
#define _P(TYPE, PREFIX, DELIM, NAME)           \
    TYPE get_##PREFIX##DELIM##NAME() const      \
    { return PREFIX##DELIM##NAME; };
    PROPERTIES
private:
    void load_props();
    float get_one_prop(const string &prefix, const string &name);
    void add_cities();
    void make_infection_prob();
};

#endif
