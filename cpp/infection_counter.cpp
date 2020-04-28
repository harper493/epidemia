#include "infection_counter.h"
#include "person.h"

void infection_counter::add_person(person *p)
{
    population += 1;
    susceptible += 1;
}

void infection_counter::infect_one(person *p)
{
    if (p->is_susceptible()) {
        susceptible -= 1;
    } else {
        debug_assert(p->is_gestating());
        gestating -= 1;
    }
    infected += 1;
    total_infected += 1;
    debug_assert(susceptible<1000000);
}

void infection_counter::gestate_one(person *p)
{
    debug_assert(p->is_susceptible());
    gestating += 1;
    susceptible -= 1;
    debug_assert(susceptible<1000000);
}

void infection_counter::immunise_one(person *p)
{
    debug_assert(p->is_susceptible());
    susceptible -= 1;
    immune += 1;
    debug_assert(susceptible<1000000);
}

void infection_counter::recover_one(person *p)
{
    debug_assert(p->is_infected());
    infected -= 1;
    recovered += 1;
    debug_assert(susceptible<1000000);
}

void infection_counter::kill_one(person *p)
{
    debug_assert(p->is_infected());
    infected -= 1;
    dead += 1;
    debug_assert(susceptible<1000000);
}

