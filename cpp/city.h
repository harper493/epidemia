#ifndef __CITY
#define __CITY

#include "common.h"
#include "cluster.h"
#include "person.h"
#include "geometry.h"
#include "chooser.h"

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
        vector<cluster*> my_clusters;
        cluster *root;
        chooser<cluster,float> my_chooser;
    };
    typedef map<cluster_type*,cluster_family> cluster_map_t;
private:
    string name;
    world *my_world;
    size_t index;
    U32 target_pop;
    point my_location;
    float exposure = 0;
    float exposure_per_person = 0;
    float pop_ratio = 0;
    float size = 0;
    U32 cluster_count = 0;
    U32 susceptible_cluster_count;
    U32 untouched_cluster_count;
    cluster_map_t my_cluster_families;
    vector<person*> my_people;
    vector<cluster*> my_clusters;
    vector<neighbor> my_neighbors;
    chooser<neighbor,float> neighbors_by_distance;
    chooser<neighbor,float> neighbors_by_appeal;
    static size_t next_index;
public:
    city(const string &name, world *w, U32 target_pop, const point &loc);
    void finalize();
    const string &get_name() const { return name; };
    void reset_location(const point &pt) { my_location = pt; };
    void reset();
    point get_random_location() const;
    void add_people();
    float distance(const city *other) const;
    city *get_random_neighbor() const;
    city *get_destination() const;
    bool is_close(const city *other) const;
    void expose();
    float get_exposure() { return exposure; };
    U32 get_population() const { return my_people.size(); };
    U32 get_leaf_cluster_count() const { return cluster_count; };
    U32 get_untouched_cluster_count() const { return untouched_cluster_count; };
    U32 get_susceptible_cluster_count() const { return susceptible_cluster_count; };
    void visit_all_clusters(cluster::visitor_fn fn);
    void visit_leaf_clusters(cluster::visitor_fn fn);
    world *get_world() const { return my_world; };
    void one_day_1();
    void one_day_2();
    void one_day_3();
    string show() const;
};

#endif
