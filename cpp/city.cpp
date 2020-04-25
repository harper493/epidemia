#include "city.h"
#include "random.h"
#include "world.h"
#include "properties.h"
#include "utility.h"
#include <tgmath.h>

/************************************************************************
 * Static data
 ***********************************************************************/

size_t city::next_index = 0;

/************************************************************************
 * Constructor
 ***********************************************************************/

city::city(const string &n, world *w, U32 tp, const point &loc)
    : name(n), my_world(w), index(next_index++), target_pop(tp), my_location(loc)
{
    my_people.reserve(target_pop);
}

/************************************************************************
 * finalize - called when all cities have been created and populated,
 * to finish up construction
 ***********************************************************************/

void city::finalize()
{
    //
    // Set size
    //
    size = my_world->make_city_size(population);
    //
    // Set infection factors
    //
    pop_ratio = my_world->get_city_pop_ratio(population);
    exposure_per_person = my_world->get_infection_prob()
        * my_world->get_props()->get_numeric("city.exposure")
        * pop_ratio;
    //
    // Build neighbor data
    //
    my_neighbors.clear();
    my_neighbors.reserve(my_world->get_cities().size());
    for (city *other : my_world->get_cities()) {
        if (other!=this) {
            float inv_dist = 1 / distance(other);
            float appeal = sqrt(inv_dist) * my_world->get_appeal_factor();
            my_neighbors.emplace_back(other, inv_dist, appeal);
        }
    }
    neighbors_by_distance.create(my_neighbors, bind(&neighbor::get_dist, _1));
    neighbors_by_appeal.create(my_neighbors, bind(&neighbor::get_appeal, _1));
}

/************************************************************************
 * one_day_1 - first phase of calculation for one day. 
 ***********************************************************************/

void city::one_day_1()
{
    exposure = 0;
    for (auto iter : my_cluster_families) {
        for (cluster *cl : iter.second.root->iter_all())  {
            cl->reset();
        }
    }
}

/************************************************************************
 * one_day_2 - second phase of calculation for one day. 
 ***********************************************************************/

void city::one_day_2()
{
    for (auto iter : my_cluster_families) {
        for (cluster *cl : iter.second.root->iter_all())  {
            cl->expose_parent();
        }
    }
    for (auto iter : my_cluster_families) {
        for (cluster *cl : iter.second.root->iter_pre())  {
            cl->gather_exposure();
        }
    }
}

/************************************************************************
 * one_day_3 - third phase of calculation for one day
 ***********************************************************************/

void city::one_day_3()
{
    susceptible_cluster_count = 0;
    untouched_cluster_count = 0;
    for (auto iter : my_cluster_families) {
        for (const cluster *cl : iter.second.root->iter_leaves())  {
            if (cl->is_susceptible()) {
                ++susceptible_cluster_count;
            }
            if (cl->is_untouched()) {
                ++untouched_cluster_count;
            }
        }
    }
}

/************************************************************************
 * get_random_location - pick a random location within the city. These
 * are biased to be closer to the center.
 ***********************************************************************/

point city::get_random_location() const
{
    float pos = random::get_random();
    offset off(pos*pos, 0);
    off.rotate(random::get_random() * 360);
    return my_location + off;
}


/************************************************************************
 * add_people - add people to the city, including assigning them
 * to clusters as appropriate
 ***********************************************************************/

void city::add_people()
{
    my_people.clear();
    my_people.reserve(target_pop);
    for (U32 n=1; n<=target_pop; ++n) {
        string pname = formatted("%s-P%d", name, n);
        cluster::list clusters;
        for (auto &i : my_cluster_families) {
            clusters.push_back(i.second.my_chooser.choose());
        }
        person *p = new person(pname, this, my_clusters);
        my_people.push_back(p);
    }
    
}

/************************************************************************
 * distence - return the distance to another city
 ***********************************************************************/

float city::distance(const city *other) const
{
    return my_location.distance(other->my_location);
}

/************************************************************************
 * get_random_neighbor - choose a neighbor at random, weighted by distance 
 ***********************************************************************/

city *city::get_random_neighbor() const
{
    return neighbors_by_distance.choose()->my_city;
}

/************************************************************************
 * get_destination - choose another city at random weighted by "appeal",
 * i.e. a function of distance and size.
 ***********************************************************************/

city *city::get_destination() const
{
    return neighbors_by_appeal.choose()->my_city;
}

/************************************************************************
 * is_close - return true if anothr city is so close that there
 * is an overlap
 ***********************************************************************/

bool city::is_close(const city *other) const
{
    return (size + other->size) < distance(other);
}

/************************************************************************
 * expose - expose the city to an infected person 
 ***********************************************************************/

void city::expose()
{
    exposure = add_probability(exposure, exposure_per_person);
}

/************************************************************************
 * show - return a short string describing the city
 ***********************************************************************/

string city::show() const
{
    return formatted("%s pop %d location %s", name, get_population(), my_location);
}

