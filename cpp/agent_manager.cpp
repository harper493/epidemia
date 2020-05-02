#include "agent_manager.h"

/************************************************************************
 * build - create the specified number of agents
 ***********************************************************************/

void agent_manager::build(size_t agent_count, factory_fn_t factory)
{
    my_agents.reserve(agent_count);    
    for (size_t idx=0; idx<agent_count; ++idx) {
        agent_base *ag = (factory)(this, idx, agent_count>1);
        my_agents.push_back(ag);
    }
    for (agent_base *ag : my_agents) {
        ag->start();
    }
}

/************************************************************************
 * execute - execute each step of each generation repeatedly
 ***********************************************************************/

void agent_manager::execute(agent_task_base *task)
{
    agent_run = false;
    current_task = task;
    if (my_agents.size()==1) {
        execute_sync(task);
    } else {
        while (true) {
            active_agents = my_agents.size();
            awake_agents();
            while (active_agents) {
                usleep(100);
                continue;
                unique_lock<mutex> msl(manager_mutex);
                manager_condition.wait(msl);
            }
            agent_run = false;
            if (!task->next_step()) {
                break;
            }
        }
    }
    current_task = NULL;
}

/************************************************************************
 * execute_sync - non-threaded version of the above, broken out for
 * clarity
 ***********************************************************************/

void agent_manager::execute_sync(agent_task_base *task)
{
    do {
        my_agents[0]->execute(task);
    } while (task->next_step());
}

/************************************************************************
 * terminate - terminate all the threads (which should be idle
 * anyway), join them, and clean up
 ***********************************************************************/

void agent_manager::terminate()
{
    terminating = true;
    awake_agents();
    for (agent_base *ag : my_agents) {
        ag->join();
        delete ag;
    }
}

/************************************************************************
 * awake_agents - wake up all the agents and let them figure out
 * what to do
 ***********************************************************************/

void agent_manager::awake_agents()
{
    agent_run = true;
    unique_lock<mutex> asl(agent_mutex);
    agent_condition.notify_all();
}

/************************************************************************
 * agent_complete - called from the agent thread when it has completed
 * the current task
 ***********************************************************************/

void agent_manager::agent_complete()
{
    --active_agents;
    unique_lock<mutex> msl(manager_mutex);
    manager_condition.notify_all();
}

/************************************************************************
 * agent_wait - called from the agent thread to wait for something
 * interesting to happen
 ***********************************************************************/

void agent_manager::agent_wait()
{
    usleep(100);
    return;
    unique_lock<mutex> asl(agent_mutex);
    agent_condition.wait(asl);
}

/************************************************************************
 * agent_base functions
 ***********************************************************************/

agent_base::agent_base(agent_manager *am, size_t idx, bool as)
    : my_manager(am), my_index(idx), async(as)
{
}

agent_base::~agent_base()
{
}

/************************************************************************
 * start - start the thread that does the work
 ***********************************************************************/

void agent_base::start()
{
    if (async) {
        my_thread.reset(new thread(&agent_base::run, this));
    }
}

/************************************************************************
 * run - run continuously looking for something new to do
 ***********************************************************************/

void agent_base::run()
{
    auto_ptr<agent_task_base> last_task;
    while (true) {
        my_manager->agent_wait();
        if (my_manager->is_terminating()) {
            break;
        }
        const agent_task_base *t = my_manager->get_task();
        if (my_manager->agent_can_run() &&
            (last_task.get()==NULL || !last_task->equals(t))) {
            last_task.reset(t->copy());
            execute(last_task.get());
            my_manager->agent_complete();
        }
    }
}

/************************************************************************
 * join - join the thread
 ***********************************************************************/

void agent_base::join()
{
    if (async) {
        my_thread->join();
    }
}

