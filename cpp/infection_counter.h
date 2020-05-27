#ifndef __INFECTION_COUNTER
#define __INFECTION_COUNTER

#include "common.h"
#include "atomic_counter.h"
#include "person_state.h"

class infection_counter
{
protected:
    U32 population = 0;
    atomic_counter susceptible;
    atomic_counter gestating;
    atomic_counter asymptomatic;
    atomic_counter infected;
    atomic_counter recovered;
    atomic_counter immune;
    atomic_counter vaccinated;
    atomic_counter dead;
    atomic_counter total_infected;
public:
    infection_counter() { };
    void count_one(person *p, person_state new_state);
    void add_person(person *p);
    bool is_untouched() const { return susceptible==population; };
    bool is_susceptible() const { return susceptible > 0; };
    U32 get_susceptible() const { return susceptible; };
    U32 get_gestating() const { return gestating; };
    U32 get_infected() const { return infected; };
    U32 get_asymptomatic() const { return asymptomatic; };
    U32 get_total_infected() const { return total_infected; };
    U32 get_recovered() const { return recovered; };
    U32 get_vaccinated() const { return vaccinated; };
    U32 get_immune() const { return immune; };
    U32 get_dead() const { return dead; };
    void set_population(U32 p)
    {
        population = susceptible = p;
    }
};

#endif

