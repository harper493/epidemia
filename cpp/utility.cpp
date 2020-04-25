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
        if (result.empty()) {
            result += delim;
        }
        result += s;
    }
    return result;
}

