#include "agent_manager.h"
#include <chrono>

/************************************************************************
 * This file implements the agent_manager / agent system for
 * parallel threads working on the same task. It supports the
 * notion of several (no predefined limit) parallel threads each
 * working on a portion of a shared task, and staying in lockstep
 * so each thread completes step N before any proceed to step N+1.
 *
 * The base classes in this file know no more than that. All
 * knowledge of the specific tasks to be performed is in the
 * derived classes.
 *
 * Specifically there are three classes here:
 *
 * -- agent_manager - creates and manages the agent threads,
 *                    and synchroizes them
 * -- agent -         base class for the worker threads
 * -- agent_task -    completely abstract class which represents
 *                    the task being performed
 *
 * An implementation needs to do the following:
 *
 * -- derive from agent a class which actually does something.
 *    It does this by implementing the execute() function
 *    which will be called successively to implement the
 *    task steps defined by the agent_task derivd class
 * -- derive from agent_task a class which describes the
 *    task to be performed. The implementation must define
 *    the function:
 *
 *    -- next_step - advances to the next step of the task.
 *                   Returns true iff the work should continue,
 *                   false, if it should stop
 *
 * Implementation Notes
 * --------------------
 *
 * -- we use condition variables to synchronise the manager
 *    and the agents, but there are too many race conditions
 *    to make this perfect, so as a backstop we run a short
 *    (100 uS) timer.
 ***********************************************************************/

/************************************************************************
 * build - create the specified number of agents. The factory function
 * must return a new object of the agent-derived class.
 ***********************************************************************/

void agent_manager::build(size_t agent_count, factory_fn_t factory)
{
    my_agents.reserve(agent_count);    
    for (size_t idx=0; idx<agent_count; ++idx) {
        agent *ag = (factory)(this, idx, agent_count>1);
        my_agents.push_back(ag);
    }
    for (agent *ag : my_agents) {
        ag->start();
    }
}

/************************************************************************
 * execute - execute each step of each generation repeatedly
 ***********************************************************************/

void agent_manager::execute(agent_task *task)
{
    current_task = task;
    agent_run = false;
    if (my_agents.size()==1) {
        execute_sync(task);
    } else {
        while (true) {
            active_agents = my_agents.size();
            awake_agents();
            while (active_agents) {
                unique_lock<mutex> msl(manager_mutex);
                manager_condition.wait_for(msl, std::chrono::microseconds(100));
            }
            agent_run = false;
            if (task->next_step()) {
                ++task->step_number;
            } else {
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

void agent_manager::execute_sync(agent_task *task)
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
    for (agent *ag : my_agents) {
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
    unique_lock<mutex> msl(manager_mutex);
    --active_agents;
    manager_condition.notify_all();
}

/************************************************************************
 * agent_wait - called from the agent thread to wait for something
 * interesting to happen
 ***********************************************************************/

void agent_manager::agent_wait()
{
    unique_lock<mutex> asl(agent_mutex);
    agent_condition.wait_for(asl, std::chrono::microseconds(100));
}

/************************************************************************
 * agent functions
 ***********************************************************************/

agent::agent(agent_manager *am, size_t idx, bool as)
    : my_manager(am), my_index(idx), async(as)
{
}

/************************************************************************
 * start - start the thread that does the work
 ***********************************************************************/

void agent::start()
{
    if (async) {
        my_thread.reset(new thread(&agent::run, this));
    }
}

/************************************************************************
 * run - run continuously looking for something new to do
 ***********************************************************************/

void agent::run()
{
    S32 last_step = -1;
    while (true) {
        my_manager->agent_wait();
        if (my_manager->is_terminating()) {
            break;
        }
        const agent_task *t = my_manager->get_task();
        if (my_manager->agent_can_run() &&
            (t->step_number > last_step)) {
            last_step = t->step_number;
            execute(t);
            my_manager->agent_complete();
        }
    }
}

/************************************************************************
 * join - join the thread
 ***********************************************************************/

void agent::join()
{
    if (async) {
        my_thread->join();
    }
}

