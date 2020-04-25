#ifndef __PROPERTIES
#define __PROPERTIES

#include "common.h"

class properties
{
public:
    typedef vector<string> prop_name_t;
    struct wild_property
    {
        string raw_form;
        regex re_form;
        string value;
        U32 wild_count=0;
        wild_property(const string &s, const string &r, const string &v, U32 wc)
            : raw_form(s), re_form(r), wild_count(wc) { };
    };
    struct property_value
    {
        string value;
        const wild_property *wild = 0;
        property_value() { };
        property_value(const string & str, const wild_property *w) : value(str), wild(w) { };
    };
    typedef map<string, property_value> property_map_t;
private:
    mutable property_map_t my_properties;
    vector<wild_property> wild_properties;
public:
    void add_property(const string &str);
    void add_properties(const vector<string> &props);
    bool add_from_file(const string &filename);
    float get_numeric(const vector<string> &pname, float dflt=0) const;
    string get(const vector<string> &prop, const string &dflt="") const;
    float get_numeric(const string &pname, float dflt=0) const;
    string get(const string &prop, const string &dflt="") const;
private:
    const wild_property *find_wild(const string &name) const;
};

#endif
