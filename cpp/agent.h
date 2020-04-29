#ifndef __AGENT
#define __AGENT

#include "common.h"
#include "person.h"
#include "double_list.h"

class agent
{
public:
    typedef double_list<person> my_list_t;
private:
    string name;
    boost::thread my_thread;
    bool async = true;
    vector<city*> my_cities;
    my_list_t susceptibles;
    my_list_t gestatings;
    my_list_t infecteds;
public:
    agent(const string &n, bool as) : name(n), async(as) { };
    void run() { };
    void add_city(city *c);
    void one_day_first(day_number day);
    void one_day_second(day_number day);
};

#endif
