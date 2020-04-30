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
    person *visitee = get_visitee(); // will be me if no travel
    switch(my_state) {
    case state::susceptible:
        {
            float risk = visitee->get_city()->get_exposure();
            for (auto *cl : visitee->my_clusters) {
                risk += cl->get_exposure(day);
            }
            if (risk>0) {
                float r = random::get_random();
                if (r < risk) {
                    result = true;
                    if (r < risk * get_world()->get_auto_immunity()) {
                        immunise(day);
                    } else {
                        gestate(day);
                    }
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
            visitee->get_city()->expose(my_city);
            for (auto *cl : visitee->my_clusters) {
                cl->expose(day, this);
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
 * get_visitee - get the person we are going to be today. If we have
 * not travelled, it's us, otherwise pick a random person
 * from a random city.
 ***********************************************************************/

person *person::get_visitee()
{
    person *result = this;
    if (mobility>0) {
        if (random::get_random() < mobility) {
            city *c = my_city->get_destination();
            result = c->get_random_person();
        }
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


