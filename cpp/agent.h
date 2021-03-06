#ifndef __AGENT
#define __AGENT

#include "common.h"
#include "person.h"
#include "double_list.h"
#include "agent_manager.h"
#include "enum_helper.h"
#include "utility.h"

class epidemia_agent : public agent
{
public:
    typedef double_list<person> my_list_t;
private:
    string name;
    world *my_world;
    size_t max_pop;
    vector<city*> my_cities;
    my_list_t susceptibles;
    my_list_t gestatings;
    my_list_t infecteds;
public:
    epidemia_agent(agent_manager *am, size_t idx, bool async, world *w);
    ~epidemia_agent() { };
    void execute(const agent_task *task) override;
    void add_cities(const vector<city*> &cities, U32 max_population);
    void build_clusters();
    void populate_cities();
    void init_day(day_number day);
    void expose(day_number day);
    void middle(day_number day);
    void infect(day_number day);
    void finalize_day(day_number day);
    static epidemia_agent *factory(agent_manager *am, size_t idx, bool async, world *w);
};

/************************************************************************
 * task for this epidemia_agent
 ***********************************************************************/

class epidemia_task : public agent_task
{
public:
    enum operations {
        op_first,
        op_build_clusters=1,
        op_populate,          // populate my cities
        op_pre_init_last,
        op_init_day,
        op_expose,
        op_middle,
        op_infect,
        op_finalize_day,
        op_last,                // must be last
    };
    struct timer
    {
        time_duration total_time;
        U32 events = 0;
    };
private:
    world *my_world;
    day_number day = 0;
    operations operation = op_build_clusters;
    array<timer, ((int)op_last)> timers;
    ptime step_start_time;
public:
    epidemia_task(world *w)
        : my_world(w), step_start_time(ptime_now()) { };
    bool next_step() override;
    string show_operation(operations op) const;
    string show_timers() const;
    static operations next_operation(operations op)
    {
        return static_cast<operations>(((int)op) + 1);
    }
friend class epidemia_agent;
};

DECLARE_SHOW_ENUM(epidemia_task::operations)

#endif
