#ifndef __CLUSTER
#define __CLUSTER

#include "common.h"
#include "infection_counter.h"
#include "geometry.h"
#include "prefetcher.h"

class cluster_type
{
public:
    typedef map<string,cluster_type*> cluster_type_map_t;
private:
    string name;
    U32 min_pop;
    U32 max_pop;
    U32 average_pop;
    float influence;
    U32 nest_min;
    U32 nest_max;
    U32 nest_average;
    float same_city;
    float nest_influence;
    float size_rms;
    float proximality;
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

class cluster : public infection_counter
{
public:
    typedef function<void(cluster*)> visitor_fn;
    typedef vector<cluster*> list;
    typedef map<cluster_type, list> list_map;
    static const int prefetch_depth = 8;
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
    const cluster_type *my_type; // keep these three in the first cache line
    day_number foreign_exposure_day = 0;
    float foreign_exposure = 0;
    day_number child_exposure_day = 0;
    float child_exposure = 0;
    day_number member_exposure_day;
    float member_exposure;
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
    static map<string,cluster_type> cluster_types;
    static vector<string> clsuter_type_names;
public:
    cluster(const string &n, const cluster_type *t, city *c, U32 sz, U16 d, const point &loc)
        : name(n), my_type(t), my_city(c), size(sz), depth(d), location(loc) { };
    void reset();
    void expose(day_number day, const person *p);
    U32 get_size() const { return size; };
    const cluster_type *get_type() const { return my_type; };
    U16 get_depth() const { return depth; };
    float get_member_exposure(day_number day);
    const point &get_location() const { return location; };
    void add_person(person *p);
    void add_child(cluster *cl);
    void expose_parent(day_number day);
    void gather_exposure(day_number day);
    void set_exposure_parent(cluster *cl);
    bool is_leaf() const { return depth==0; };
    bool is_full() const { return my_children.size() > size+1; };
    bool is_foreign_exposure() const { return my_parent != my_exposure_parent; };
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
};

inline void cluster::prefetch_one(cluster *cl, int slot)
{
    switch(slot) {
    case prefetch_depth-1:
        prefetch(cl);
        break;
    case prefetch_depth-4:
        prefetch(cl->my_parent);
        break;
    default:
        break;
    }
}

#endif
