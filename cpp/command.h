#ifndef __COMMAND
#define __COMMAND

#include "common.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class command
{
private:
    po::variables_map values;
    map<string,string> props;
    int thread_count = 0;
    string output_file;
    bool csv = false;
    vector<string> props_files;
public:
    bool parse(int argc, const char **argv);
    int get_thread_count() const { return thread_count; };
    string get_output_file() const { return output_file; };
    bool get_csv() const { return csv; };
    const vector<string> &get_props_files() const { return props_files; };
    const map<string,string> &get_props() const { return props; };
private:
    string as_string(const string &name);
};

#endif
