#include "agent.h"
#include "city.h"
#include "world.h"
#include "person.h"
#include "formatted.h"

/************************************************************************
 * static data
 ***********************************************************************/

SHOW_ENUM(epidemia_task::operations)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, populate)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, pre_init_last)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, init_day)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, expose)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, middle)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, infect)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, finalize_day)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, last)
SHOW_ENUM_END(epidemia_task::operations)

/************************************************************************
 * constructor
 ***********************************************************************/

epidemia_agent::epidemia_agent(agent_manager *am, size_t idx, bool async, world *w)
    : agent(am, idx, async), name(lexical_cast<string>(idx)), my_world(w)
{
    w->build_agent(this);
}

/************************************************************************
 * factory - create a new epidemia_agent
 ***********************************************************************/

epidemia_agent *epidemia_agent::factory(agent_manager *am, size_t idx, bool async, world *w)
{
    return new epidemia_agent(am, idx, async, w);
}


/************************************************************************
 * execute - execute the task as described
 ***********************************************************************/

void epidemia_agent::execute(const agent_task *task_base)
{
    const epidemia_task *task = reinterpret_cast<const epidemia_task*>(task_base);
#if 0
    std::cout << formatted("epidemia_agent %2d day %4d operation %s\n",
                           my_index, task->day, task->show_operation());
#endif
    switch (task->operation) {
    case epidemia_task::op_populate:
        populate_cities();
        break;
    case epidemia_task::op_init_day:
        init_day(task->day);
        break;
    case epidemia_task::op_expose:
        expose(task->day);
        break;
    case epidemia_task::op_middle:
        middle(task->day);
        break;
    case epidemia_task::op_infect:
        infect(task->day);
        break;
    case epidemia_task::op_finalize_day:
        finalize_day(task->day);
        break;
    default:                    // shoudl never happen
        break;
    }
}

/************************************************************************
 * add_cities - add a city, and add its people to the
 * appropriate lists. If 'spacing' is not one, then we
 * just every n'th person since we are sharing the city with
 * other epidemia_agents.
 ***********************************************************************/

void epidemia_agent::add_cities(const vector<city*> &cities, U32 max_population)
{
    for (city *c : cities) {
        my_cities.push_back(c);
    }
    max_pop = max_population;
}

/************************************************************************
 * populate_cities - add people to each city I own. For the first city,
 * add only max_pop if it is non-zero. If I have more than once city
 * fully populate the others.
 ***********************************************************************/

void epidemia_agent::populate_cities()
{
    bool first = true;
    for (city *c : my_cities) {
        U32 pop = c->get_target_pop();
        if (first && max_pop>0) {
            pop = min(pop, max_pop);
        }
        first = false;
        unique_lock<mutex> sl(c->get_agent_lock());
        c->build_clusters();
        for (int i=0; i<pop; ++i) {
            person *p = c->add_person();
            susceptibles.insert(p);
        }
    }
}

/************************************************************************
 * init_day - do one day for each city that we own, part 1
 ***********************************************************************/

void epidemia_agent::init_day(day_number day)
{
    for (city *c : my_cities) {
        unique_lock<mutex> sl(c->get_agent_lock());
        c->init_day(day);
    }
}

/************************************************************************
 * expose - expose all our infected people. We have to take the
 * city's lock when we do this, but not for processing the gestatings
 ***********************************************************************/

void epidemia_agent::expose(day_number day)
{
    gestatings.reset();
    for (person *p : gestatings) {
        p->one_day(day);
        if (p->is_gestating()) {
            gestatings.insert(p);
        } else {
            infecteds.insert(p);
        }
    }
    city *prev_city = NULL;
    auto_ptr<lock_guard<mutex>> the_lock;
    infecteds.reset();
    for (person *p : infecteds) {
        city *this_city = p->get_city();
        if (this_city!=prev_city) {
            the_lock.reset(new lock_guard<mutex>(this_city->get_agent_lock()));
            prev_city = this_city;
        }
        p->one_day(day);
        if (p->is_infected()) {
            infecteds.insert(p);
        }
    }
}

/************************************************************************
 * middle
 ***********************************************************************/

void epidemia_agent::middle(day_number day)
{
    for (city *c : my_cities) {
        unique_lock<mutex> sl(c->get_agent_lock());
        c->middle_day(day);
    }
}

/************************************************************************
 * infect - scan the susceptibles and see if any of them get infected.
 * We don't need to take any locks for this.
 ***********************************************************************/

void epidemia_agent::infect(day_number day)
{
    susceptibles.reset();
    for (person *p : susceptibles) {
        p->one_day(day);
        if (p->is_susceptible()) {
            susceptibles.insert(p);
        } else if (p->is_gestating()) {
            gestatings.insert(p);
        } else if (p->is_infected()) {
            infecteds.insert(p);
        }
    }
}

/************************************************************************
 * finalize_day - clean up after everything else
 ***********************************************************************/

void epidemia_agent::finalize_day(day_number day)
{
    for (city *c : my_cities) {
        unique_lock<mutex> sl(c->get_agent_lock());
        c->finalize_day(day);
    }
}

/************************************************************************
 * epidemia_task functions
 ***********************************************************************/

bool epidemia_task::next_step()
{
    bool result = true;
    operation = static_cast<operations>(((int)operation) + 1);
    if (operation>=op_last || operation==op_pre_init_last) {
        operation = op_init_day;
        ++day;
        result = my_world->end_of_day();
    }
    return result;
}

/************************************************************************
 * epidemia_task functions
 ***********************************************************************/

string epidemia_task::show_operation() const
{
    return enum_helper<epidemia_task::operations>().str(operation);
}
