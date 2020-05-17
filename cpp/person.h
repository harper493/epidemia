#ifndef __PERSON
#define __PERSON

#include "common.h"
#include "enum_helper.h"
#include "geometry.h"
#include "cluster.h"
#include "allocator.h"
#include "sized_array.h"

class person : public bintr::list_base_hook<bintr::link_mode<bintr::auto_unlink>>,
               public allocator_user<person>
{
public:
    enum class state: U8 {
        susceptible = 0,
        vaccinated,
            gestating,
            infected,
            recovered,
            dead,
            immune,
    };
    typedef bintr::list<person, bintr::constant_time_size<false>> list;
    typedef sized_array<cluster*,cluster_type::max_cluster_types> cluster_array_t;
    static const int prefetch_depth = 10;
private:
    string name;
    state my_state = state::susceptible;
    city *my_city;
    point my_location;
    cluster_array_t my_clusters;
    float mobility = 0;
    day_number infected_time;
    day_number next_transition;
public:
    person(const string &n, city *c, const point &loc, const cluster::list &clusters);
    const string &get_name() const { return name; };
    state get_state() const { return my_state; };
    city *get_city() const { return my_city; };
    cluster_array_t &get_clusters() { return my_clusters; };
    float get_mobility() const { return mobility; };
    bool is_susceptible() const { return my_state==state::susceptible; };
    bool is_gestating() const { return my_state==state::gestating; };
    bool is_infected() const { return my_state==state::infected; };
    bool is_recovered() const { return my_state==state::recovered; };
    bool is_immune() const { return my_state==state::immune; };
    bool is_vaccinated() const { return my_state==state::vaccinated; };
    bool is_dead() const { return my_state==state::dead; };
    bool one_day(day_number day);
    void force_infect(day_number day);
    void vaccinate();
    void kill();
    string show() const;
    world *get_world() const;
    static person* factory(const string &n, city *c, const point &loc, const cluster::list &clusters);
private:
    void gestate(day_number day);
    void infect(day_number day);
    void recover();
    void immunise(day_number day);
    person *get_visitee();
public:
    static void prefetch(person *p, int slot)
    {
        switch(slot) {
        case prefetch_depth-1:
            ::prefetch(p);
            break;
        case 4:
            for (cluster *cl : p->my_clusters) {
                ::prefetch_n<2>(cl);
            }
            break;
        default:
            break;
        }
    }
} _cache_aligned;

DECLARE_ALLOCATOR(person)

#endif
    
