#ifndef __AGENT_MANAGER
#define __AGENT_MANAGER

#include "common.h"
#include "atomic_counter.h"
#include <condition_variable>

using std::condition_variable;
using std::unique_lock;

class agent;
class agent_task;

class agent_manager
{
public:
    typedef function<agent*(agent_manager*,size_t,bool)> factory_fn_t;
private:
    vector<agent*> my_agents;
    agent_task *current_task = NULL;
    bool terminating = false;
    bool agent_run = false;
    atomic_counter active_agents;    
    condition_variable manager_condition;
    mutex manager_mutex;
    condition_variable agent_condition;
    mutex agent_mutex;
public:
    agent_manager() { };
    agent_manager(size_t agent_count, factory_fn_t factory)
    {
        build(agent_count, factory);
    }
    ~agent_manager()
    {
        terminate();
    }
    void build(size_t agent_count, factory_fn_t factory);
    void execute(agent_task *task);
    void terminate();
private:
    void agent_complete();
    void execute_sync(agent_task *task);
    void awake_agents();
    void agent_wait();
    const agent_task *get_task() const { return current_task; };
    bool is_terminating() const { return terminating; };
    bool agent_can_run() const { return agent_run; };
friend class agent;
};

class agent
{
protected:
    agent_manager *my_manager;
    size_t my_index;
    auto_ptr<thread> my_thread;
    bool async;
public:
    agent(agent_manager *am, size_t idx, bool as);
    virtual ~agent();
    virtual void execute(const agent_task *task) = 0;
    size_t get_index() const { return my_index; };
    void start();
    void run();
    void join();
friend class agent_manager;
};

class agent_task
{
 private:
 public:
    virtual ~agent_task() { };
    virtual bool next_step() = 0;
    virtual bool equals(const agent_task *other) const = 0;
    virtual agent_task *copy() const = 0;
};

#endif
