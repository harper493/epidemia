#include "agent.h"
#include "city.h"
#include "world.h"
#include "person.h"
#include "formatted.h"

/************************************************************************
 * This class implements the agent (see agent_manager.cpp) responsible for
 * performing the world class functions. It is driven by a single
 * function, execute(), which dispatches according to the task described in
 * the epdiemia_task object. That describes a day, and the job to be done
 * for that day.
 *
 * There are several phases to the operation:
 *
 * -- on day 0 only, we run populate_cities
 * -- on subseequent days, there are the following steps. They are 
 *    performed in lockstep, supervised by the agent manager, such
 *    that all agents h have completed step N before any o fthem
 *    start step N+1
 * -- initialise our cities for the day
 * -- spread infection from our infected population
 * -- manage the infection within clusters within the cities (middle() )
 * -- expose our susceptible population
 * -- finalize the city operations
 *
 * Each agent is responsible for a subset of the total population, which
 * it keeps in three lists: susceptible, destating and infected. Once
 * they have recovered, there is nothing for us to do so we just 
 * forget about them. Each agent creates its own people in
 * populate_cities().
 *
 * For themost part each city is handeld by exactly one agent. But the 
 * largest cities need to be split, with a subset of the population
 * handled by different agents. This requires some careful
 * concurrency control to allow maximum parallelism without
 * breaking anything.
 ***********************************************************************/

/************************************************************************
 * static data
 ***********************************************************************/

SHOW_ENUM(epidemia_task::operations)
    SHOW_ENUM_VAL(epidemia_task::operations::op_, build_clusters)
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
                           my_index, task->day, task->show_operation(operation));
#endif
    switch (task->operation) {
    case epidemia_task::op_build_clusters:
        build_clusters();
        break;
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
 * appropriate lists.
 ***********************************************************************/

void epidemia_agent::add_cities(const vector<city*> &cities, U32 max_population)
{
    for (city *c : cities) {
        my_cities.push_back(c);
    }
    max_pop = max_population;
}

/************************************************************************
 * build_clusters - build clusters for my cities
 ***********************************************************************/

void epidemia_agent::build_clusters()
{
    for (city *c : my_cities) {
        c->build_clusters();
    }
}

/************************************************************************
 * populate_cities - add people to each city I own.
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
        for (int i=0; i<pop; ++i) {
            person *p = c->make_person();
            c->add_person(p);
            if (p->is_susceptible()) {
                susceptibles.insert(p);
            }
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
    timer &t = timers[((int)operation)];
    t.total_time += ptime_now() - step_start_time;
    ++t.events;
    operation = next_operation(operation);
    if (operation>=op_last || operation==op_pre_init_last) {
        operation = op_init_day;
        ++day;
        result = my_world->end_of_day();
    }
    step_start_time = ptime_now();
    return result;
}

/************************************************************************
 * epidemia_task functions
 ***********************************************************************/

string epidemia_task::show_operation(operations op) const
{
    return enum_helper<epidemia_task::operations>().str(op);
}

/************************************************************************
 * show_timers - generate a string showing the step timers
 ***********************************************************************/

string epidemia_task::show_timers() const
{
    string result;
    operations op = op_first;
    while (op!=op_last) {
        const timer &t = timers[op];
        if (t.events) {
            join_to(result,
                    formatted("%14s: %8.3f mS",
                              show_operation(op),
                              ((float)t.total_time.total_microseconds())/(1000 * t.events)),
                    "\n");
        }
        op = next_operation(op);
    }
    return result;
}

