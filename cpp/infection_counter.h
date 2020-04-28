#ifndef __INFECTION_COUNTER
#define __INFECTION_COUNTER

#include "common.h"

class infection_counter
{
protected:
    U32 population = 0;
    U32 susceptible = 0;
    U32 gestating = 0;
    U32 infected = 0;
    U32 recovered = 0;
    U32 immune = 0;
    U32 dead = 0;
    U32 total_infected = 0;
public:
    infection_counter() { };
    void infect_one(person *p);
    void gestate_one(person *p);
    void recover_one(person *p);
    void immunise_one(person *p);
    void kill_one(person *p);
    void add_person(person *p);
    bool is_untouched() const { return susceptible==population; };
    bool is_susceptible() const { return susceptible > 0; };
    U32 get_susceptible() const { return susceptible; };
    U32 get_gestating() const { return gestating; };
    U32 get_infected() const { return infected; };
    U32 get_total_infected() const { return total_infected; };
    U32 get_recovered() const { return recovered; };
    U32 get_immune() const { return immune; };
    U32 get_dead() const { return dead; };
};

#endif

