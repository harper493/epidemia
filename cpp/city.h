#ifndef __CITY
#define __CITY

#include "common.h"
#include "cluster.h"
#include "person.h"
#include "geometry.h"
#include "chooser.h"
#include "log_output.h"

class city : public infection_counter
{
public:
    class neighbor
    {
    private:
        city *my_city;
        float inv_distance;
        float appeal;
    public:
        neighbor(city *c, float d, float a)
            : my_city(c), inv_distance(d), appeal(a) { };
        bool less_by_distance(const neighbor *other) { return inv_distance<other->inv_distance; };
        bool less_by_appeal(const neighbor *other) { return appeal<other->appeal; };
        float get_dist() const { return inv_distance; };
        float get_appeal() const { return appeal; };
    friend class city;
    };
    struct cluster_family
    {
        const cluster_type *my_type = NULL;
        cluster *root = NULL;
        vector<cluster::list> clusters;
        chooser<cluster,float> my_chooser;
        bool active = false;
        cluster_family(const cluster_type *type_) : my_type(type_) { };
        cluster::list &get_leaf_clusters() { return clusters[0]; };
    };
    typedef map<const cluster_type*,cluster_family*> cluster_map_t;
private:
    string name;
    world *my_world;
    size_t index;
    U32 target_pop;
    point my_location;
    float exposure = 0;
    float exposure_per_person = 0;
    float foreign_exposure;
    float pop_ratio = 0;
    float size = 0;
    U32 cluster_count = 0;
    U32 susceptible_cluster_count;
    U32 untouched_cluster_count;
    cluster_map_t my_cluster_families;
    vector<person*> my_people;
    vector<cluster*> my_clusters;
    vector<neighbor*> my_neighbors;
    chooser<neighbor,float> neighbors_by_distance;
    chooser<neighbor,float> neighbors_by_appeal;
    mutex foreign_exposure_lock;
    mutex agent_lock;
    atomic_counter person_number = 0;
    day_number init_day_no = 0;
    day_number middle_day_no = 0;
    day_number finalize_day_no = 0;
    mutex my_mutex;
    static size_t next_index;
public:
    city(const string &name, world *w, U32 target_pop, const point &loc);
    void finalize();
    const string &get_name() const { return name; };
    void reset_location(const point &pt) { my_location = pt; };
    void reset();
    point get_random_location() const;
    void add_people();
    void add_person(person *p);
    person *make_person();
    template<class COLL>
    void add_people(const COLL &coll);
    void build_clusters();
    float distance(const city *other) const;
    person *get_random_person() const;
    city *get_random_neighbor() const;
    city *get_destination() const;
    bool is_close(const city *other) const;
    void expose(city *owner);
    void foreign_expose();
    float get_exposure() { return exposure; };
    bool build_one_cluster_family();
    bool middle_one_cluster(day_number day);
    U32 get_population() const { return my_people.size(); };
    U32 get_target_pop() const { return target_pop; };
    U32 get_target_population() const { return target_pop; };
    const point &get_location() const { return my_location; };
    U32 get_leaf_cluster_count() const { return cluster_count; };
    U32 get_untouched_cluster_count() const { return untouched_cluster_count; };
    U32 get_susceptible_cluster_count() const { return susceptible_cluster_count; };
    cluster *get_random_parent_cluster(cluster *cl) const;
    const vector<person*> &get_people() const { return my_people; };
    world *get_world() const { return my_world; };
    mutex &get_agent_lock() { return agent_lock; };
    void init_day(day_number day);
    void middle_day(day_number day);
    void finalize_day(day_number day);
    void log(log_output &logger) const;
    static const log_output::column_defs &get_columns();
friend class world;
};

/************************************************************************
 * Inline functions
 ***********************************************************************/

template<class COLL>
inline void city::add_people(const COLL &coll)
{
    unique_lock<mutex> lg(my_mutex);
    for (person *p : const_cast<COLL&>(coll)) {
        if (p->get_city()==this) {
            add_person(p);
        }
    }
}


#endif
