#include "agent.h"
#include "city.h"
#include "person.h"
#include "formatted.h"

/************************************************************************
 * add_city - add a city, and also add any infectibles it has
 ***********************************************************************/

void agent::add_city(city *c)
{
    my_cities.push_back(c);
    for (person *p : c->get_people()) {
        if (p->is_infected()) {
            infecteds.insert(p);
        } else if (p->is_susceptible()) {
            susceptibles.insert(p);
        } else if (p->is_gestating()) {
            gestatings.insert(p);
        }
    }
}

/************************************************************************
 * one_day_first - do one day for each city that we own, part 1: gathering
 * exposure
 ***********************************************************************/

void agent::one_day_first(day_number day)
{
    for (city *c : my_cities) {
        c->one_day_1();
    }
    gestatings.reset();
    for (person *p : gestatings) {
        p->one_day(day);
        if (p->is_gestating()) {
            gestatings.insert(p);
        } else {
            infecteds.insert(p);
        }
    }
    infecteds.reset();
    for (person *p : infecteds) {
        p->one_day(day);
        if (p->is_infected()) {
            infecteds.insert(p);
        }
    }
}

/************************************************************************
 * one_day_second - for each person we control, expose them to
 * risk
 ***********************************************************************/

void agent::one_day_second(day_number day)
{
    for (city *c : my_cities) {
        c->one_day_2();
    }
    susceptibles.reset();
    for (person *p : susceptibles) {
        p->one_day(day);
        if (p->is_susceptible()) {
            susceptibles.insert(p);
        } else if (p->is_gestating()) {
            gestatings.insert(p);
        }
    }
    for (city *c : my_cities) {
        c->one_day_3();
    }
#if 0
    std::cout << formatted("                                                      "
                           "inf %d => %d gest %d => %d susc %d => %d\n",
                           infecteds.size(), infecteds.next_size(), gestatings.size(),
                           gestatings.next_size(), susceptibles.size(), susceptibles.next_size());
#endif
}

