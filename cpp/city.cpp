#include "city.h"
#include "random.h"
#include "world.h"
#include "properties.h"
#include "utility.h"
#include "formatted.h"
#include <tgmath.h>
#include <boost/range/adaptors.hpp>

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
            my_neighbors.push_back(new neighbor(other, inv_dist, appeal));
        }
    }
    neighbors_by_distance.create(my_neighbors, bind(&neighbor::get_dist, _1));
    neighbors_by_appeal.create(my_neighbors, bind(&neighbor::get_appeal, _1));
    //
    // Make some leaf clusters have foreign parents
    //
    for (auto i : my_cluster_families) {
        cluster_family *cf = i.second;
        if (cf->my_type->same_city < 1) {
            const auto &leafs = cf->get_leaf_clusters();
            for (U32 target_count = leafs.size() * (1 - cf->my_type->same_city);
                 target_count > 0;
                 --target_count) {
                while (true) {
                    cluster *victim = random::uniform_choice(leafs);
                    if (!victim->is_foreign_exposure()) {
                        city *foreign = my_world->get_random_city();
                        if (foreign!=this) {
                            cluster *foreign_cluster = foreign->get_random_parent_cluster(victim);
                            if (foreign_cluster) {
                                victim->set_exposure_parent(foreign_cluster);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

/************************************************************************
 * one_day_1 - first phase of calculation for one day. 
 ***********************************************************************/

void city::one_day_1()
{
    exposure = 0;
    foreign_exposure = 0;
}

/************************************************************************
 * one_day_2 - second phase of calculation for one day. 
 ***********************************************************************/

void city::one_day_2()
{
    day_number day = my_world->get_day();
    exposure = add_probability(exposure, foreign_exposure);
    for (auto iter : my_cluster_families) {
        cluster_family *cf = iter.second;
        for (cluster::list &cll : cf->clusters) {
            for (cluster *cl : cluster::cluster_prefetcher(cll))  {
                cl->expose_parent(day);
            }
        }
        for (cluster::list &cll : cf->clusters | boost::adaptors::reversed) {
            for (cluster *cl : cluster::cluster_prefetcher(cll))  {
                cl->gather_exposure(day);
            }
        }
    }
}

/************************************************************************
 * one_day_3 - third phase of calculation for one day
 ***********************************************************************/

void city::one_day_3()
{
    if (false) {
        susceptible_cluster_count = 0;
        untouched_cluster_count = 0;
        for (auto iter : my_cluster_families) {
            cluster::cluster_prefetcher pf(iter.second->get_leaf_clusters());
            for (cluster *cl : pf) {
                if (cl->is_susceptible()) {
                    ++susceptible_cluster_count;
                }
                if (cl->is_untouched()) {
                    ++untouched_cluster_count;
                }
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
 * get_random_person
 ***********************************************************************/

person *city::get_random_person() const
{
    return random::uniform_choice(my_people);
}

/************************************************************************
 * build_clusters - build the nest of clusters for each cluster type
 ***********************************************************************/

void city::build_clusters()
{
    for (const auto &iter : cluster_type::get_cluster_types()) {
        const cluster_type *clt = iter.second;
        cluster_family *cf = new cluster_family(clt);
        cf->root = clt->make_clusters(this);
        my_cluster_families[clt] = cf;
        //
        // Build cluster lists
        //
        for (auto iter : my_cluster_families) {
            cluster_family *cf = iter.second;
            for (cluster *cl : cf->root->iter_all()) {
                while (cl->get_depth() >= cf->clusters.size()) {
                    cf->clusters.emplace_back();
                }
                cf->clusters[cl->get_depth()].push_back(cl);
            }
        }
        cf->my_chooser.create(cf->get_leaf_clusters(), &cluster::get_size);
    }
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
        string pname = formatted("%s.P%d", name, n);
        point location;
        cluster::list clusters;
        for (auto &i : my_cluster_families) {
            cluster_family *cf = i.second;
            cluster *cl = cf->my_chooser.choose();
            debug_assert(cl->is_leaf());
            clusters.push_back(cl);
            if (cf->my_type->is_local()) {
                location = cl->get_location();
            }
        }
        person *p = new person(pname, this, location, clusters);
        my_people.push_back(p);
        this->infection_counter::add_person(p);
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
 * get_random_parent_cluster - given a cluster from another city,
 * choose a random cluster to be its "exposure parent". Must bein
 * the same family and at one greater depth. Return NULL if there is
 * no such cluster.
 ***********************************************************************/

cluster *city::get_random_parent_cluster(cluster *cl) const
{
    cluster *result = NULL;
    U32 depth = cl->get_depth();
    cluster_family *cf = find_in_map(my_cluster_families, cl->get_type());
    if (cf && cf->clusters.size() > depth+1) {
        result = random::uniform_choice(cf->clusters[depth+1]);
    }
    return result;
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

void city::expose(city *owner)
{
    if (owner==this) {
        exposure = add_probability(exposure, exposure_per_person);
    } else {
        foreign_expose();
    }
}

/************************************************************************
 * foreign_expose - expose the city to an infected person, for an agent
 * that does not own this city
 ***********************************************************************/

void city::foreign_expose()
{
    mutex::scoped_lock sl(foreign_exposure_lock);
    foreign_exposure = add_probability(exposure, exposure_per_person);
}

/************************************************************************
 * show - return a short string describing the city
 ***********************************************************************/

string city::show() const
{
    return formatted("%s pop %d location %s", name, get_population(), my_location);
}

