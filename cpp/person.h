#ifndef __PERSON
#define __PERSON

#include "common.h"
#include "enum_helper.h"
#include "geometry.h"
#include "cluster.h"

class person : public bintr::list_base_hook<bintr::link_mode<bintr::auto_unlink>>
{
public:
    enum class state: U8 {
        susceptible = 0,
            gestating,
            infected,
            recovered,
            dead,
            immune,
    };
    typedef bintr::list<person, bintr::constant_time_size<false>> list;
    static const int prefetch_depth = 8;
private:
    string name;
    state my_state = state::susceptible;
    city *my_city;
    point my_location;
    cluster::list my_clusters;
    float mobility = 0;
    day_number infected_time;
    day_number next_transition;
public:
    person(const string &n, city *c, const point &loc, const cluster::list &clusters);
    const string &get_name() const { return name; };
    state get_state() const { return my_state; };
    city *get_city() const { return my_city; };
    float get_mobility() const { return mobility; };
    bool is_susceptible() const { return my_state==state::susceptible; };
    bool is_gestating() const { return my_state==state::gestating; };
    bool is_infected() const { return my_state==state::infected; };
    bool is_recovered() const { return my_state==state::recovered; };
    bool is_immune() const { return my_state==state::immune; };
    bool one_day(day_number day);
    void force_infect(day_number day);
    string show() const;
    world *get_world() const;
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
        case prefetch_depth-4:
            for (cluster *cl : p->my_clusters) {
                ::prefetch_n<1>(cl);
            }
            break;
        default:
            break;
        }
    }
};

#endif
    
