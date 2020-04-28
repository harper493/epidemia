#include "person.h"
#include "random.h"
#include "city.h"
#include "world.h"

/************************************************************************
 * Constructor.
 *
 * -- place at a random location with the city
 * -- assign self to random clusters within the city
 ***********************************************************************/

person::person(const string &n, city *c, const point &loc, const cluster::list &clusters)
    : name(n), my_city(c), my_location(loc)
{
    for (cluster *cl : clusters) {
        cl->add_person(this);
        my_clusters.push_back(cl);
    }
}

/************************************************************************
 * one_day - do whatever needs doing for this person for one day,
 * based on state.
 *
 * Return true if state has changed.
 ***********************************************************************/

bool person::one_day(day_number day)
{
    bool result = false;
    switch(my_state) {
    case state::susceptible:
        {
            float r = random::get_random();
            float risk = get_today_city()->get_exposure();
            for (auto *cl : my_clusters) {
                risk += cl->get_exposure();
            }
            if (r < risk) {
                result = true;
                if (r < risk * get_world()->get_auto_immunity()) {
                    immunise(day);
                } else {
                    gestate(day);
                }
            }
        }
        break;
    case state::gestating:
        if (day >= next_transition) {
            result = true;
            infect(day);
        }
        break;
    case state::infected:
        if (day >= next_transition) {
            result = true;
            recover();
        } else {
            get_today_city()->expose(my_city);
            for (auto *cl : my_clusters) {
                cl->expose(this);
            }
        }
        break;
    default:                    // nothing to do for the other states
        break;
    }
    return result;
}

/************************************************************************
 * gestate - start the gestation period
 ***********************************************************************/

void person::gestate(day_number day)
{
    infected_time = day;
    next_transition = day + get_world()->get_gestation_interval();
    my_city->gestate_one(this);
    for (auto *cl : my_clusters) {
        cl->gestate_one(this);
    }
    my_state = state::gestating;
}

/************************************************************************
 * infect - transition for gestating to infected
 ***********************************************************************/

void person::infect(day_number day)
{
    next_transition = day + get_world()->get_recovery_interval();
    my_city->infect_one(this);
    for (auto *cl : my_clusters) {
        cl->infect_one(this);
    }
    my_state = state::infected;
}

/************************************************************************
 * recover - transition for infected to recovered
 ***********************************************************************/

void person::recover()
{
    my_city->recover_one(this);
    for (auto *cl : my_clusters) {
        cl->recover_one(this);
    }
    my_state = state::recovered;
}

/************************************************************************
 * immunise - transition to immunised
 ***********************************************************************/

void person::immunise(day_number day)
{
    my_city->immunise_one(this);
    for (auto *cl : my_clusters) {
        cl->immunise_one(this);
    }
    my_state = state::immune;
}

/************************************************************************
 * force_infect - force infection without gestation, used when
 * creating the initial infected population
 ***********************************************************************/

void person::force_infect(day_number day)
{
    infected_time = day;
    infect(day);
}

/************************************************************************
 * get_today_city - get the city for today, depending on whether
 * we have travelled or not
 ***********************************************************************/

city *person::get_today_city()
{
    city *result = NULL;
    if (random::get_random() < get_world()->get_travel_prob()) {
        result = my_city->get_destination();
    } else {
        result = my_city;
    }
    return result;
}

/************************************************************************
 * get_world - get our world from our city
 ***********************************************************************/

world *person::get_world() const
{
    return my_city->get_world();
}


