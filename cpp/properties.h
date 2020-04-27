#ifndef __PROPERTIES
#define __PROPERTIES

#include "common.h"

class properties
{
public:
    typedef vector<string> prop_name_t;
    class wild_property;
    struct property_value
    {
        string name;
        string value;
        const wild_property *wild = NULL;
        property_value() { };
        property_value(const string &n, const string &str, const wild_property *w)
            : name(n), value(str), wild(w) { };
        virtual ~property_value() { };
        const string &get_name() const { return name; };
        string get_value() const;
        vector<string> get_elements() const;
        virtual bool is_wild() const { return wild!=NULL; };
    };
    struct wild_property : public property_value
    {
        regex re_form;
        U32 wild_count=0;
        wild_property(const string &s, const string &r, const string &v, U32 wc)
            : property_value(s, v, this), re_form(r), wild_count(wc) { };
        virtual bool is_wild() const override { return true; };
    };
    typedef map<string, property_value*> property_map_t;
    class const_iterator : public std::forward_iterator_tag
    {
    public:
        typedef property_value* value_type;
        typedef property_value*& reference_type;
        typedef std::forward_iterator_tag iterator_category;
    private:
        const properties *my_props = NULL;
        property_map_t::const_iterator my_iter;
    public:
        const_iterator() { };
        const_iterator(const properties *p);
        const_iterator(const const_iterator &other)
            : my_props(other.my_props), my_iter(other.my_iter)
        { };
        const_iterator &operator=(const const_iterator &other)
        {
            my_props = other.my_props;
            my_iter = other.my_iter;
            return *this;
        };
        bool operator==(const const_iterator &other) const;
        bool operator!=(const const_iterator &other) const { return !(*this==other); };
        value_type operator*() const { return my_iter->second; };
        value_type operator->() const { return this->operator*(); };
        const_iterator &operator++();
        const_iterator operator++(int) { const_iterator result=*this; ++*this; return result; };
    private:
        bool ended() const { return my_iter==my_props->my_properties.end(); };
    friend class properties;
    };
private:
    mutable property_map_t my_properties;
    vector<wild_property*> wild_properties;
public:
    void add_property(const string &str);
    void add_properties(const vector<string> &props);
    bool add_from_file(const string &filename);
    float get_numeric(const vector<string> &pname, float dflt=0) const;
    string get(const vector<string> &prop, const string &dflt="") const;
    float get_numeric(const string &pname, float dflt=0) const;
    string get(const string &prop, const string &dflt="") const;
    const_iterator begin() const { return const_iterator(this); };
    const_iterator end() const { return const_iterator(); };
private:
    const wild_property *find_wild(const string &name) const;
friend class const_iterator;
};

inline string properties::property_value::get_value() const
{
    return wild ? wild->value : value;
}

#endif
