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

size_t city::next_index = 1;

/************************************************************************
 * Constructor
 ***********************************************************************/

city::city(const string &n, world *w, U32 tp, const point &loc)
    : name(n), my_world(w), index(next_index++), target_pop(tp), my_location(loc)
{
    my_people.reserve(target_pop);
    size = my_world->make_city_size(target_pop);
}

/************************************************************************
 * finalize - called when all cities have been created and populated,
 * to finish up construction
 ***********************************************************************/

void city::finalize()
{
    //
    // Set infection factors
    //
    pop_ratio = my_world->get_city_pop_ratio(target_pop);
    exposure_per_person = my_world->get_infection_prob()
        * my_world->get_props()->get_numeric("city.exposure")
        * pop_ratio;
    //
    // populate my_people from the clusters
    //
    {
        lock_guard<mutex> lg(my_mutex);
        for (auto i : my_cluster_families) {
            cluster_family *cf = i.second;
            if (cf->my_type->is_local()) {
                for (cluster *cl : cluster::cluster_prefetcher(cf->get_leaf_clusters())) {
                    for (person *p : cl->get_people()) {
                        my_people.push_back(p);
                    }
                }
                break;
            }
        }
    }
    this->infection_counter::set_population(my_people.size());
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
 * init_day - first phase of calculation for one day. 
 ***********************************************************************/

void city::init_day(day_number day)
{
    if (init_day_no != day) {
        init_day_no = day;
        exposure = 0;
        foreign_exposure = 0;
        for (auto iter : my_cluster_families) {
            cluster_family *cf = iter.second;
            cf->active = false;
        }
    }
}

/************************************************************************
 * middle_day - second phase of calculation for one day. 
 ***********************************************************************/

void city::middle_day(day_number day)
{
    {
        lock_guard<mutex> lg(my_mutex);
        if (middle_day_no != day) {
            middle_day_no = day;
            exposure = add_probability(exposure, foreign_exposure);
            for (auto iter : my_cluster_families) {
                cluster_family *cf = iter.second;
            }
        }
    }
    while (middle_one_cluster(day)) { };
}

/************************************************************************
 * middle_one_cluster - consolidate the data for a single cluster,
 * which we choose. The idea is that different threads can call this
 * function and each of them will get to do a single family. Return
 * true iff there is potentially more to do.
 ***********************************************************************/

bool city::middle_one_cluster(day_number day)
{
    cluster_family *cf = NULL;
    {
        lock_guard<mutex> lg(my_mutex);
        for (auto iter : my_cluster_families) {
            cf = iter.second;
            if (!cf->active) {
                cf->active = true;
                break;
            } else {
                cf = NULL;
            }
        }
    }
    if (cf) {
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
    return cf!=NULL;
}

/************************************************************************
 * finalize_day - third phase of calculation for one day
 ***********************************************************************/

void city::finalize_day(day_number day)
{
    if (finalize_day_no != day) {
        finalize_day_no = day;
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
 * build_clusters - build the nest of clusters for each cluster type. This
 * may be called for multiple threads, so don't do anything once
 * the clusters are present.
 ***********************************************************************/

void city::build_clusters()
{
    while (true) {
        cluster_family *cf = NULL;
        {
            lock_guard<mutex> lg(my_mutex);
            for (const auto &iter : cluster_type::get_cluster_types()) {
                const cluster_type *clt = iter.second;
                if (my_cluster_families.find(clt)==my_cluster_families.end())  {
                    cf = new cluster_family(clt);
                    my_cluster_families[clt] = cf;
                    break;
                } else {
                    cf = NULL;
                }
            }
        }
        if (cf) {
            cf->root = cf->my_type->make_clusters(this);
            for (cluster *cl : cf->root->iter_all()) {
                while (cl->get_depth() >= cf->clusters.size()) {
                    cf->clusters.emplace_back();
                }
                cf->clusters[cl->get_depth()].push_back(cl);
            }
            cf->my_chooser.create(cf->get_leaf_clusters(), &cluster::get_size);
        } else {
            break;              // all done
        }
    }
}

/************************************************************************
 * add_person - add one person.
 ***********************************************************************/

void city::add_person(person *p)
{
    infection_counter::add_person(p);
    for (cluster *cl : p->get_clusters()) {
        cl->add_person(p);
    }
    if (random::get_random() < my_world->get_vaccination()) {
        p->vaccinate();
    }
}

/************************************************************************
 * make_person - create one person for this city
 ***********************************************************************/

person *city::make_person()
{
    size_t my_number = person_number;
    ++person_number;
    char pname[64];             // big enough!
    strcpy(pname, name.c_str());
    char *offset = &pname[name.size()];
    *offset++ = '.';
    itoa(person_number, offset);
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
    person *p = person::factory(pname, this, location, clusters);
    return p;
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
    lock_guard<mutex> sl(foreign_exposure_lock);
    foreign_exposure = add_probability(exposure, exposure_per_person);
}

/************************************************************************
 * log - describe self into a log file
 ***********************************************************************/

log_output::column_defs city_columns{
    { "10s", "name" },
    { "6d", "index" },
    { "11d", "population" },
    { "8.3f", "size" },
    { "20s", "location" },
};

const log_output::column_defs &city::get_columns()
{
    return city_columns;
}

void city::log(log_output &logger) const
{
    logger.put_line(name, index, target_pop, size, my_location);
}

