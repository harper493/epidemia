#include "properties.h"
#include "utility.h"

/************************************************************************
 * add_property - take a string and turn it into a property. The string
 * mustbe of the form:
 *
 * property.name = xxx
 *
 * Any individual name component can be a *.
 ***********************************************************************/

void properties::add_property(const string &str)
{
    regex rx("\\s*(\\S+?)\\s*=\\s*(\\S+)\\s*(?:#.*)?");
    smatch s;
    if (regex_match(str, s, rx)) {
        string name = s[1];
        string value = s[2];
        if (name[0]=='#') {
            return;
        }
        vector<string> name_parts = split(name, ".");
        U32 wc = 0;
        for (string &np : name_parts) {
            if (np=="*") {
                ++wc;
                np = ".*?";
            }
        }
        if (wc) {
            string re = join(name_parts, "\\.");
            wild_property *w = new wild_property(name, re, value, wc);
            wild_properties.emplace_back(w);
            my_properties[name] = w;
        } else {
            my_properties[name] = new property_value(name, value, NULL);
        }
    }
}

/************************************************************************
 * add_properties - add multple properties in one go.
 ***********************************************************************/

void properties::add_properties(const vector<string> &props)
{
    for (const string &prop : props) {
        add_property(prop);
    }
}


/************************************************************************
 * add_from_file - add properties from a file. Return true if
 * the file was OK, false if not.
 ***********************************************************************/

bool properties::add_from_file(const string &filename)
{
    std::ifstream file(filename);
    if (file.fail()) {
        return false;
    }
    while (!file.eof()) {
        string line;
        std::getline(file, line);
        add_property(line);
    }
    return true;
}

/************************************************************************
 * get - try to find a property, returning its value if found. If not
 * found, return the default if supplied, else an empty string.
 *
 * If we don't have a definite or cached value, try to find a matching
 * wildcard value.
 ***********************************************************************/

string properties::get(const string &prop, const string &dflt) const
{
    string result = dflt;
    auto iter = my_properties.find(prop);
    if (iter==my_properties.end()) {
        const wild_property *wp = find_wild(prop);
        if (wp) {
            my_properties[prop] = new property_value(prop, "", wp);
            result = wp->value;
        }
    } else if (iter->second->wild) {
        result = iter->second->wild->value;
    } else {
        result = iter->second->value;
    }
    return result;
}

string properties::get(const vector<string> &prop, const string &dflt) const
{
    return get(join(prop), dflt);
}


/************************************************************************
 * get_numeric - return a numeric (float) result. If the property
 * is missing or if the result is not a valid number, return the
 * given default or 0.
 ***********************************************************************/

float properties::get_numeric(const string &pname, float dflt) const
{
    float result = dflt;
    string str_value = get(pname);
    if (!str_value.empty()) {
        try {
            result = lexical_cast<float>(str_value);
        } catch (...) { };
    }
    return result;
}

float properties::get_numeric(const vector<string> &pname, float dflt) const
{
    return get_numeric(join(pname), dflt);
}

/************************************************************************
 * find_wild - try to match the given name among the wildcard names we
 * have. If we find more than one match, return the one with the fewest
 * wild elements.
 ***********************************************************************/

const properties::wild_property *properties::find_wild(const string &name) const
{
    const wild_property *result = NULL;
    for (const wild_property *wp : wild_properties) {
        smatch m;
        if (regex_match(name, m, wp->re_form)
            && (result==NULL || wp->wild_count < result->wild_count)) {
            result = wp;            
        }
    }
    return result;
}

/************************************************************************
 * Iterator functions
 ***********************************************************************/

properties::const_iterator::const_iterator(const properties *p)
    : my_props(p), my_iter(p->my_properties.begin())
{
}

bool properties::const_iterator::operator==(const const_iterator &other) const
{
    return (my_props==NULL && other.my_props==NULL)
        || my_iter==other.my_iter;
}
    
properties::const_iterator &properties::const_iterator::operator++()
{
    if (my_props) {
        while (my_iter!=my_props->my_properties.end()) {
            ++my_iter;
        }
        if (my_iter!=my_props->my_properties.end()) {
            return *this;
        } else {
            my_props = NULL;
        }
    }
    return *this;
}
