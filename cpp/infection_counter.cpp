#include "infection_counter.h"
#include "person.h"

void infection_counter::add_person(person *p)
{
    population += 1;
    susceptible += 1;
}

void infection_counter::count_one(person *p, person_state new_state)
{
    switch (p->get_state()) {
    case person_state::pst_susceptible:
        --susceptible;
        break;
    case person_state::pst_vaccinated:
        --vaccinated;
        break;
    case person_state::pst_gestating:
        --gestating;
        break;
    case person_state::pst_asymptomatic:
        --asymptomatic;
        break;
    case person_state::pst_infected:
        --infected;
        break;
    case person_state::pst_recovered:
        --recovered;
        break;
    case person_state::pst_dead:
        --dead;
        break;
    case person_state::pst_immune:
        --immune;
        break;
    }
    switch (new_state) {
    case person_state::pst_susceptible:
        ++susceptible;
        break;
    case person_state::pst_vaccinated:
        ++vaccinated;
        break;
    case person_state::pst_gestating:
        ++gestating;
        break;
    case person_state::pst_asymptomatic:
        ++total_infected;
        ++asymptomatic;
        break;
    case person_state::pst_infected:
        if (p->get_state()==person_state::pst_susceptible) {
            ++total_infected;
        }
        ++infected;
        break;
    case person_state::pst_recovered:
        ++recovered;
        break;
    case person_state::pst_dead:
        ++dead;
        break;
    case person_state::pst_immune:
        ++immune;
        break;
    }    
}

