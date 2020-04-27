#include "cluster.h"
#include "city.h"
#include "world.h"
#include "formatted.h"
#include "utility.h"
#include "properties.h"

/************************************************************************
 * Static data
 ***********************************************************************/

cluster_type::cluster_type_map_t cluster_type::cluster_types;

/************************************************************************
 * reset - reset to base condition for a new day
 ***********************************************************************/

void cluster::reset()
{
    exposure = 0;
}

/************************************************************************
 * expose - expose a leaf cluster to a single infected person
 ***********************************************************************/

void cluster::expose()
{
    exposure = add_probability(exposure,
                               get_world()->get_infection_prob() * my_type->influence);
}

/************************************************************************
 * get_exposure - get the exposure for a single member of the cluster
 ***********************************************************************/

float cluster::get_exposure()
{
    return exposure * my_type->influence;
}

/************************************************************************
 * add_person - add a person to this cluster
 ***********************************************************************/

void cluster::add_person(person *p)
{
    my_people.push_back(p);
}

/************************************************************************
 * add_child - add a child cluster
 ***********************************************************************/

void cluster::add_child(cluster *cl)
{
    debug_assert(cl->depth+1==depth);
    my_children.push_back(cl);
}

/************************************************************************
 * set_parent - set our parent
 ***********************************************************************/

void cluster::set_parent(cluster *p)
{
    my_parent = p;
    p->add_child(this);
}

/************************************************************************
 * expose_parent - pass on our exposure to parent
 ***********************************************************************/

void cluster::expose_parent()
{
    my_parent->exposure = add_probability(my_parent->exposure, exposure * my_type->nest_influence);
}

/************************************************************************
 * gather_exposure - inherit the exposure from parent
 ***********************************************************************/

void cluster::gather_exposure()
{
    exposure = add_probability(exposure, my_parent->exposure * my_type->nest_influence);
}

/************************************************************************
 * get_world - get our world via our city. Can't be in the .h file for
 * header circularity.
 ***********************************************************************/

world *cluster::get_world()
{
    return my_city->get_world();
}

/************************************************************************
 * iterator constructor. Set things up as ++ is expecting.
 ***********************************************************************/

cluster::iterator::iterator(cluster *r, bool lo, bool po)
    : my_state(state::st_pre), leaf_only(lo), pre_order(po)
{
    my_iterators.push_back(r->my_children.begin());
    while (!is_ended() && (leaf_only && !((*this)->is_leaf()))) {
        advance();
    }
}

/************************************************************************
 * iterator ==
 ***********************************************************************/

bool cluster::iterator::operator==(const cluster::iterator &other) const
{
    return my_iterators.empty()
        ? other.my_iterators.empty()
        : other.my_iterators.empty()
          ? false
          : my_iterators.size()==other.my_iterators.size()
            && my_iterators.back()==other.my_iterators.back();
}

/************************************************************************
 * iterator dereference
 ***********************************************************************/

cluster::iterator::value_type cluster::iterator::operator*() const
{
    if (is_ended()) {
        return NULL;
    } else {
        debug_assert(!my_iterators.empty());
        return *(my_iterators.back());
    }
}


/************************************************************************
 * iterator++ - advance the iterator, skipping ones we don't want
 ***********************************************************************/

cluster::iterator &cluster::iterator::operator++()
{
    while (!is_ended()) {
        advance();
        if (!is_ended() && (!leaf_only || (*this)->is_leaf())) {
            debug_assert(!leaf_only || (*this)->is_leaf());
            break;
        }
    }
    return *this;
}

/************************************************************************
 * iterator::advance - advance by a single entry in the nest.
 *
 * This is extremely tricky code to implement something simple,
 * which in Python would be a recursive generator. The Python code
 * code looks like this, but in C++ we have to hand code
 * the recursion stack.
 *
 * def child_iter(self) :
 *     if pre_order and (depth==0 or not leaf_only) :
 *         yield
 *     for c in children :
 *         yield from c.child_iter()
 *     if not pre_order and (depth==0 or not lead_only) :
 *         yield
 *
 ***********************************************************************/

void cluster::iterator::advance()
{
    ++count;
    while (true) {
        assert(is_ended() || !my_iterators.empty());
        switch (my_state) {
        case state::st_pre:
            if (pre_order && ((*this)->depth==0 || !leaf_only)) {
                my_state = state::st_start;
                return;
            };
            my_state = state::st_start;
            break;
        case state::st_start:
            {
                cluster *cl = **this;
                if (!cl->my_children.empty()) {
                    my_iterators.push_back(cl->my_children.begin());
                    my_state = state::st_pre;
                } else {
                    my_state = state::st_post;
                }
            }
            break;
        case state::st_mid:
            {
                auto &iter = my_iterators.back();
                cluster *parent = (*iter)->my_parent;
                iter++;
                if (iter==parent->my_children.end()) {
                    my_iterators.pop_back();
                    if (my_iterators.empty()) {
                        my_state = state::st_end;
                    } else {
                        my_state = state::st_post;
                    }
                } else {
                    my_state = state::st_pre;
                }
            }
            break;
        case state::st_post:
            if (!pre_order && ((*this)->depth==0 || !leaf_only)) {
                my_state = state::st_mid;
                return;
            }
            my_state = state::st_mid;
            break;
        case state::st_end:
        default:
            return;
        }
    }
}

/************************************************************************
 * refresh - refresh all the parameters from the properties
 ***********************************************************************/

#define refresh_one(prop) prop = props->get_numeric({"cluster", name, #prop})

void cluster_type::refresh(properties *props)
{
    refresh_one(min_pop);
    refresh_one(max_pop);
    refresh_one(average_pop);
    refresh_one(same_city);
    refresh_one(influence);
    refresh_one(nest_min);
    refresh_one(nest_max);
    refresh_one(nest_average);
    refresh_one(same_city);
    refresh_one(nest_max);
    refresh_one(proximality);
}

/************************************************************************
 * refresh_all - refresh all cluster types
 ***********************************************************************/

void cluster_type::refresh_all(properties *props)
{
    for (auto iter : cluster_types) {
        iter.second->refresh(props);
    }
}

/************************************************************************
 * find_cluster_type - get a cluster type by name. If it doesn't exist,
 * create it.
 ***********************************************************************/

cluster_type *cluster_type::find_cluster_type(const string &n)
{
    cluster_type *result = NULL;
    auto iter = cluster_types.find(n);
    if (iter==cluster_types.end()) {
        result = new cluster_type(n);
        cluster_types[n] = result;
    } else {
        result = iter->second;
    }
    return result;        
}

/************************************************************************
 * cluster_type member functions
 ***********************************************************************/

/************************************************************************
 * build - read the properties and determine what exists
 ***********************************************************************/

void cluster_type::build(properties *props)
{
    for (const auto *prop : *props) {
        if (!prop->is_wild()) {
            auto elems = prop->get_elements();
            if (elems[0]=="cluster"
                && elems.size()>=3
                && cluster_types.find(elems[1])==cluster_types.end()) {
                cluster_types[elems[1]] = new cluster_type(elems[1]);
            }
        }
    }
    refresh_all(props);
}

/************************************************************************
 * make_clusters - build a nest of clusters according to the given
 * parameters for the given city. Return the root cluster,
 * from which all the others can be found using the iterator.
 ***********************************************************************/

cluster *cluster_type::make_clusters(city *c) const
{
    vector<vector<cluster*>> clusters;
    float this_min = min_pop;
    float this_max = max_pop;
    float this_average = average_pop;
    for (size_t depth=0; true; ++depth) {
        U32 count = max(1, (depth==0 ? c->get_target_population() : clusters[depth-1].size()) / this_average);
        if (count<5) {
            count = 1;          // make this the top
        }
        this_max = min(this_max, ((count * this_average) - this_min) / count);
        vector<U32> sizes;
        if (count==1) {
            sizes.push_back(clusters.back().size());
        } else {
            random::reciprocal recip(this_min, this_max, count, this_average);
            sizes = recip.get_values_int();
        }
        clusters.emplace_back();
        clusters[depth].reserve(count);
        for (size_t i=0; i<count; ++i) {
            string clname = formatted("%s.%s.%d.%d", c->get_name(), name, depth, i);
            assert(sizes[i]<1000000);
            clusters[depth].push_back(new cluster(clname, this, c, sizes[i], depth, c->get_random_location()));
        }
        if (depth > 0) {
            chooser<cluster, U32> choose(clusters[depth], &cluster::get_size);
            for (cluster *child : clusters[depth-1]) {
                cluster *parent = NULL;
                if (count==1) {
                    parent = clusters[depth][0];
                } else {
                    do {
                        parent = choose.choose();
                    } while (parent->is_full());
                }
                child->set_parent(parent);
            }
        }
        if (count==1) {         // we're done
            break;
        }
        this_min = nest_min;
        this_max = nest_max;
        this_average = nest_average;
    }
    return clusters.back()[0];
}

