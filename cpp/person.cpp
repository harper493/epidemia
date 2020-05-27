#include "person.h"
#include "random.h"
#include "city.h"
#include "world.h"
#include "cluster.h"

/************************************************************************
 * Static data
 ***********************************************************************/

DEFINE_ALLOCATOR(person)

SHOW_ENUM(person_state)
    SHOW_ENUM_VAL(person_state::pst_, susceptible)
    SHOW_ENUM_VAL(person_state::pst_, vaccinated)
    SHOW_ENUM_VAL(person_state::pst_, gestating)
    SHOW_ENUM_VAL(person_state::pst_, asymptomatic)
    SHOW_ENUM_VAL(person_state::pst_, infected)
    SHOW_ENUM_VAL(person_state::pst_, recovered)
    SHOW_ENUM_VAL(person_state::pst_, dead)
    SHOW_ENUM_VAL(person_state::pst_, immune)
SHOW_ENUM_END(person_state)

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
        my_clusters.push_back(cl);
    }
    mobility = get_world()->make_mobility();
}

/************************************************************************
 * The factory is needed to work around issues with the template
 * static allocator pointer.
 ***********************************************************************/

person *person::factory(const string &n, city *c, const point &loc, const cluster::list &clusters)
{
    return new person(n, c, loc, clusters);
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
    case person_state::pst_susceptible:
        {
            float risk = visitee->get_city()->get_exposure();
            for (auto *cl : visitee->my_clusters) {
                risk += cl->get_member_exposure(day);
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
    case person_state::pst_gestating:
        if (day >= next_transition) {
            result = true;
            asymptomatic(day);
        }
        break;
    case person_state::pst_asymptomatic:
        if (day >= next_transition) {
            result = true;
            infect(day);
        }
        break;
    case person_state::pst_infected:
        if (day >= next_transition) {
            result = true;
            if (random::get_random() < get_world()->get_mortality()) {
                kill(day);
            } else {
                recover(day);
            }
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
 * set_state - adjust the state counts for the cioty and myh
 * clusters
 ***********************************************************************/

void person::set_state(person_state new_state)
{
    my_city->count_one(this, new_state);
    for (auto *cl : my_clusters) {
        cl->count_one(this, new_state);
    }
    my_state = new_state;
}

/************************************************************************
 * gestate - start the gestation period
 ***********************************************************************/

void person::gestate(day_number day)
{
    infected_time = day;
    next_transition = day + get_world()->get_gestation_interval();
    set_state(person_state::pst_gestating);
}

/************************************************************************
 * asymptomatic - transition for gestating to asymptomatic
 ***********************************************************************/

void person::asymptomatic(day_number day)
{
    next_transition = day + get_world()->get_asymptomatic_interval();
    set_state(person_state::pst_asymptomatic);
}

/************************************************************************
 * infect - transition for asymptomatic to infected
 ***********************************************************************/

void person::infect(day_number day)
{
    next_transition = day + get_world()->get_recovery_interval();
    set_state(person_state::pst_infected);
}

/************************************************************************
 * recover - transition for infected to recovered
 ***********************************************************************/

void person::recover(day_number day)
{
    set_state(person_state::pst_recovered);
}

/************************************************************************
 * immunise - transition to immunised
 ***********************************************************************/

void person::immunise(day_number day)
{
    set_state(person_state::pst_immune);
}

/************************************************************************
 * vaccinate - transition to vaccinated
 ***********************************************************************/

void person::vaccinate(day_number day)
{
    set_state(person_state::pst_vaccinated);
}

/************************************************************************
 * kill - transition to dead
 ***********************************************************************/

void person::kill(day_number day)
{
    set_state(person_state::pst_dead);
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


