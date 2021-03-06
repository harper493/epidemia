#ifndef __CLUSTER
#define __CLUSTER

#include "common.h"
#include "infection_counter.h"
#include "geometry.h"
#include "prefetcher.h"
#include "spinlock.h"
#include "allocator.h"

#define CLUSTER_PROPERTIES \
    _CP(U32, min_pop) \
    _CP(U32, max_pop) \
    _CP(U32, average_pop) \
    _CP(float, influence) \
    _CP(U32, nest_min) \
    _CP(U32, nest_max) \
    _CP(U32, nest_average) \
    _CP(U32, max_depth) \
    _CP(float, singleton) \
    _CP(float, singleton_influence) \
    _CP(float, same_city) \
    _CP(float, nest_influence) \
    _CP(float, size_rms) \
    _CP(float, proximality) \
    
class cluster_type
{
public:
    typedef map<string,cluster_type*> cluster_type_map_t;
    static const int max_cluster_types = 5;
private:
    string name;

#undef _CP
#define _CP(TYPE, PROP) TYPE PROP; \
    TYPE base_##PROP;

    CLUSTER_PROPERTIES
    float exposure;
    static cluster_type_map_t cluster_types;
public:
    cluster_type(const string &n) : name(n) { };
    const string &get_name() const { return name; };
    void refresh(properties *props);
    cluster *make_clusters(city *c) const;
    void finalize(world *w);
    bool is_local() const { return proximality==1; };
    static cluster_type *find_cluster_type(const string &n);
    static void refresh_all(properties *props);
    static void build(world *w);
    static const cluster_type_map_t &get_cluster_types() { return cluster_types; };
private:
    void build_one(world *w);
friend class city;
friend class cluster;
friend class world;
};

class cluster_user
{
private:
    cluster *my_cluster = NULL;
    float influence = 1;
public:
    cluster_user() { };
    cluster_user(cluster *c, float i)
        : my_cluster(c), influence(i) { };
    cluster_user(const cluster_user &other)
        : my_cluster(other.my_cluster), influence(other.influence) { };
    float get_exposure(day_number day) const;
    void expose(day_number day, person *p);
    cluster *get_cluster() const { return my_cluster; };
public:
    typedef vector<cluster_user> list;
};

class cluster : public infection_counter,
                public allocator_user<cluster>
{
public:
    typedef function<void(cluster*)> visitor_fn;
    typedef vector<cluster*> list;
    typedef map<cluster_type, list> list_map;
    static const int prefetch_depth = 6;
    class iterator : public std::forward_iterator_tag
    {
    public:
        typedef cluster* value_type;
        typedef cluster*& reference_type;
        typedef std::forward_iterator_tag iterator_category;
        enum class state: U8 {
            st_pre,
            st_start,
            st_mid,
            st_post,
            st_end
        };
    private:
        state my_state;
        vector<cluster::list::iterator> my_iterators;
        bool leaf_only = false;
        bool pre_order = true;
        size_t count = 0;
    public:
        iterator() : my_state(state::st_end) { };
        iterator(cluster *r, bool lo, bool po);
        iterator(const iterator &other)
            : my_iterators(other.my_iterators),
              leaf_only(other.leaf_only),
              pre_order(other.pre_order)
        { };
        iterator &operator=(const iterator &other)
        {
            my_iterators = other.my_iterators;
            leaf_only = other.leaf_only;
            pre_order = other.pre_order;
            return *this;
        };
        bool operator==(const iterator &other) const;
        bool operator!=(const iterator &other) const { return !(*this==other); };
        value_type operator*() const;
        value_type operator->() const { return this->operator*(); };
        iterator &operator++();
        iterator operator++(int) { iterator result=*this; ++*this; return result; };
    private:
        void advance();
        bool is_ended() const { return my_state==state::st_end; };
    };
    class iterator_controller
    {
    private:
        cluster *my_cluster=NULL;
        bool leaf_only = false;
        bool pre_order = true;
    public:
        iterator_controller() { };
        iterator_controller(cluster *cl, bool lo, bool po)
            : my_cluster(cl), leaf_only(lo), pre_order(po) { };
        iterator begin() const { return iterator(my_cluster, leaf_only, pre_order); }
        iterator end() const { return iterator(); }
    };
private:
    day_number exposure_day = 0;
    float exposure = 0;
    day_number member_exposure_day;
    float member_exposure;
    day_number foreign_exposure_day = 0;
    float foreign_exposure = 0;
    day_number child_exposure_day = 0;
    float child_exposure = 0;
    const cluster_type *my_type; // keep these in the first cache line
    string name;
    cluster *my_parent;
    cluster *my_exposure_parent;
    city *my_city;
    U32 size = 0;
    U16 depth = 0;
    vector<person*> my_people;
    vector<cluster*> my_children;
    float influence = 0;
    point location;
    bool has_children = false;
    mutex foreign_lock;
    spinlock my_lock;
    static map<string,cluster_type> cluster_types;
    static vector<string> clsuter_type_names;
public:
    cluster(const string &n, const cluster_type *t, city *c, U32 sz, U16 d, const point &loc)
        : name(n), my_type(t), my_city(c), size(sz), depth(d), location(loc) { };
    void reset();
    void expose(day_number day, const person *p, float influence=1);
    U32 get_size() const { return size; };
    const cluster_type *get_type() const { return my_type; };
    U16 get_depth() const { return depth; };
    float get_member_exposure(day_number day);
    const point &get_location() const { return location; };
    cluster *get_parent() const { return my_parent; };
    void add_person(person *p);
    void add_child(cluster *cl);
    void expose_parent(day_number day);
    void gather_exposure(day_number day);
    void set_exposure_parent(cluster *cl);
    bool is_leaf() const { return depth==0; };
    bool is_full() const { return my_children.size() > size+1; };
    bool is_foreign_exposure() const { return my_parent != my_exposure_parent; };
    const vector<person*> &get_people() const { return my_people; };
    iterator_controller iter_all() { return iterator_controller(this, false, false); };
    iterator_controller iter_pre() { return iterator_controller(this, false, true); };
    iterator_controller iter_leaves() { return iterator_controller(this, true, false); };    
    static cluster_type get_cluster_type(const string &type_name);
    static const string &get_cluster_name(cluster_type &t);
    static void prefetch_one(cluster *cl, int slot);
private:
    world *get_world();
    void set_parent(cluster *p);
    void add_child_exposure(day_number day, cluster *cl);
    float get_exposure(day_number day);
    void add_exposure(day_number day, city *c, float e);
    float get_child_exposure(day_number day);
public:
    typedef prefetcher<cluster::list, prefetch_depth, &prefetch_one> cluster_prefetcher;
friend class cluster_type;
friend class cluster_user;
} _cache_aligned;

inline void cluster::prefetch_one(cluster *cl, int slot)
{
    switch(slot) {
    case prefetch_depth-1:
        prefetch(cl);
        break;
    case prefetch_depth/2:
        prefetch(cl->my_parent);
        break;
    default:
        break;
    }
}

#endif
