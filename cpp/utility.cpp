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
 * add_one_to_string - add one to the numeric value represented by
 * the last characters of the string.
 *
 * This isn't really kosher but it's a lot faster which matters when
 * you are giving numeric, in sequence numbers to tens of millions
 * of things (like people and clusters).
 ***********************************************************************/

void add_one_to_string(string &str)
{
    char *end = const_cast<char*>(str.data() + str.size() - 1);
    char *d = end;
    const char *start = str.data();
    bool redo = false;
    while (d >= start && !redo) {
        if (*d=='9') {
            --d;
        } else if (isdigit(*d)) {
            *d++ += 1;
            while (d <= end) {
                *d++ = '0';
            }
            break;
        } else {                // reached the beginning of the numeric part
            redo = true;
        }
    }
    if (d < start || redo) {
        ++d;
        size_t offset = d - str.data();
        string new_value = str.substr(0, offset);
        new_value += '1';
        while (d <= end) {
            ++d;
            new_value += '0';
        }
        str = new_value;
    }
}

/************************************************************************
 * itoa - no longer part of stdlib but way more efficient than
 * the "modern" alternatives
 ***********************************************************************/

void itoa(U32 value, char *ptr)
{
    char buf[64];
    char *bufptr = buf;
    if (value==0) {
        *bufptr++ = '0';
    } else {
        while (value>0) {
            U32 next = value / 10;
            *bufptr++ = '0' + (value - next*10);
            value = next;
        }
    }
    while (bufptr > buf) {
        *ptr++ = *--bufptr;
    }
    *ptr++ = 0;
}


/************************************************************************
 * get_system_core_count - return the number of cores given by the
 * lscpu command. Return - if we can't get it.
 ***********************************************************************/

int get_system_core_count()
{
    int result = 0;
    regex rx("^cpu cores.*:\\s*(\\d+)");
    std::ifstream istr("/proc/cpuinfo");
    while (istr.good()) {
        string line;
        std::getline(istr, line);
        smatch s;
        if (regex_match(line, s, rx)) {
            result = max(result, lexical_cast<int>(s[1]));
        }
    }
    return result;
}

