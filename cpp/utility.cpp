#include "utility.h"

/************************************************************************
 * split - split a string into components based on the given
 * delimiters, and return the corresponding vector
 ***********************************************************************/

vector<string> split(const string &in, const string &delims)
{
    vector<string> result;
    boost::split(result, in, boost::is_any_of(delims));
    return result;
}

/************************************************************************
 * join - the well-known function
 ***********************************************************************/

string join(const vector<string> &vec, const string &delim)
{
    string result;
    for (const string &s : vec) {
        if (!result.empty()) {
            result += delim;
        }
        result += s;
    }
    return result;
}

/************************************************************************
 * trim - apply trim to each string in a vector
 ***********************************************************************/

vector<string> trim(const vector<string> &input)
{
    vector<string> result;
    for (const string &s : input) {
        result.emplace_back(s);
        boost::trim(result.back());
    }
    return result;
}

/************************************************************************
 * get_system_core_count - return the number of cores given by the
 * lscpu command. Return 0- if we can't get it.
 ***********************************************************************/

int get_system_core_count()
{
    int result = 0;
    regex rx("^processor.*:\\s*(\\d+)");
    std::ifstream istr("/proc/cpuinfo");
    while (istr.good()) {
        string line;
        std::getline(istr, line);
        smatch s;
        if (regex_match(line, s, rx)) {
            result = max(result, lexical_cast<int>(s[1]));
        }
    }
    return result + 1;
}

