#include "common.h"

#ifndef __PERSON_STATE
#define __PERSON_STATE

enum class person_state: U8 {
    pst_susceptible = 0,
    pst_vaccinated,
    pst_gestating,
    pst_asymptomatic,
    pst_infected,
    pst_recovered,
    pst_dead,
    pst_immune,
};

#endif
