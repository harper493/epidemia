#ifndef __PERSON
#define __PERSON

#include "common.h"
#include "enum_helper.h"
#include "geometry.h"
#include "allocator.h"
#include "sized_array.h"
#include "prefetch.h"
#include "common.h"
#include "cluster.h"
#include "person_state.h"

class person : public bintr::list_base_hook<bintr::link_mode<bintr::auto_unlink>>,
               public allocator_user<person>
{
public:
    typedef bintr::list<person, bintr::constant_time_size<false>> list;
    typedef sized_array<cluster*,max_cluster_types> cluster_array_t;
    static const int prefetch_depth = 10;
private:
    string name;
    person_state my_state = person_state::pst_susceptible;
    city *my_city;
    point my_location;
    cluster_array_t my_clusters;
    float mobility = 0;
    float severity = 0;
    float current_severity = 0;
    day_number infected_time;
    day_number next_transition;
public:
    person(const string &n, city *c, const point &loc, const cluster::list &clusters);
    const string &get_name() const { return name; };
    person_state get_state() const { return my_state; };
    city *get_city() const { return my_city; };
    cluster_array_t &get_clusters() { return my_clusters; };
    float get_mobility() const { return mobility; };
    bool is_susceptible() const { return my_state==person_state::pst_susceptible; };
    bool is_gestating() const { return my_state==person_state::pst_gestating; };
    bool is_infected() const { return my_state==person_state::pst_infected; };
    bool is_asymptomatic() const { return my_state==person_state::pst_asymptomatic; };
    bool is_infectious() const { return is_asymptomatic() || is_infected(); };
    bool is_recovered() const { return my_state==person_state::pst_recovered; };
    bool is_immune() const { return my_state==person_state::pst_immune; };
    bool is_vaccinated() const { return my_state==person_state::pst_vaccinated; };
    bool is_dead() const { return my_state==person_state::pst_dead; };
    bool one_day(day_number day);
    void force_infect(day_number day);
    void vaccinate(day_number day);
    void kill(day_number day);
    string show() const;
    world *get_world() const;
    static person* factory(const string &n, city *c, const point &loc, const cluster::list &clusters);
private:
    void gestate(day_number day);
    void asymptomatic(day_number day);
    void infect(day_number day);
    void recover(day_number day);
    void immunise(day_number day);
    void set_state(person_state new_state);
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

DECLARE_SHOW_ENUM(person_state)

#endif
    
