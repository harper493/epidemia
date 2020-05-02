#ifndef __AGENT
#define __AGENT

#include "common.h"
#include "person.h"
#include "double_list.h"
#include "agent_manager.h"
#include "enum_helper.h"

class agent : public agent_base
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
    agent(agent_manager *am, size_t idx, bool async, world *w);
    ~agent() { };
    void execute(const agent_task_base *task) override;
    void add_cities(const vector<city*> &cities, U32 max_population);
    void populate_cities();
    void init_day(day_number day);
    void expose(day_number day);
    void middle(day_number day);
    void infect(day_number day);
    void finalize_day(day_number day);
    static agent *factory(agent_manager *am, size_t idx, bool async, world *w);
};

/************************************************************************
 * task for this agent
 ***********************************************************************/

class agent_task : public agent_task_base
{
public:
    enum operations {
        op_populate=1,          // populate my cities
        op_pre_init_last,
        op_init_day,
        op_expose,
        op_middle,
        op_infect,
        op_finalize_day,
        op_last,                // must be last
    };
private:
    world *my_world;
    day_number day = 0;
    operations operation = op_populate;    
public:
    agent_task(world *w)
        : my_world(w) { };
    agent_task(const agent_task &other)
        : my_world(other.my_world), day(other.day), operation(other.operation) { };
    bool next_step() override;
    bool equals(const agent_task_base *other) const override
    {
        const agent_task *casted = reinterpret_cast<const agent_task*>(other);
        return day==casted->day && operation==casted->operation;
    }
    agent_task *copy() const override
    {
        return new agent_task(*this);
    }
    string show_operation() const;
friend class agent;
};

DECLARE_SHOW_ENUM(agent_task::operations)

#endif
