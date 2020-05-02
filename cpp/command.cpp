#include "command.h"
#include "formatted.h"
#include "utility.h"
#include "string_exception.h"

/************************************************************************
 * parse - parse the command line, recognising the options in the
 * code below. If there is an error, throws the corresponding exception.
 *
 * Returns true if the command should be acted upon, false if not
 * (i.e. for help).
 *
 * If it returns true, the values have been digested. The ones which
 * override properties in the props file are stored in the props dict,
 * the others are stored explicitly.
 *
 * (boost::program_options kind-sorta works but the ability to store values
 * doesn't seem to work. And the documentation is on the poor side of
 * appalling).
 ***********************************************************************/

bool command::parse(int argc, const char **argv)
{
    bool result = true;
    int initial_infected;
    po::options_description visible("The following options are available:");
    visible.add_options()
        ("auto-immunity,a",   po::value<float>(),  "level of auto immunity (0-1)")
        ("help,h",                                 "show help message")
        ("csv",                                    "output in CSV format")
        ("infectiousness,i",  po::value<float>(),  "infectiousness, typically 1-5")
        ("initial,n",         po::value<int>(),    "initial infected population")
        ("infected-cities,C", po::value<float>(),  "proportion or number of cities to infect")
        ("max_days",          po::value<int>(),    "max days to run for")
        ("min_days",          po::value<int>(),    "min days to run for")
        ("output,o",          po::value<string>(), "output file")
        ("population,p",      po::value<int>(),    "population")
        ("random,r",          po::value<float>(),  "random number seed")
        ("threads,T",         po::value<int>(),    "number of threads to use")
        ("verbosity,v",       po::value<int>(),    "how much output to give");
    po::options_description hidden("");
    hidden.add_options()
        ("positional",        po::value<vector<string>>(),       "positional arguments");
    po::positional_options_description pod;
    pod.add("positional", -1);
    po::options_description all_options("");
    all_options.add(visible).add(hidden);
    string err;
    po::store(po::command_line_parser(argc, argv).
              options(all_options).
              positional(pod).
              run(),
              values);
    if (values.count("help")) {
        std::cout << visible << std::endl;
        result = false;
    } else {
        for (auto iter : values) {
            string name = iter.first;
            if (name=="threads") {
                props["thread_count"] = as_string("threads");
            } else if (name=="verbosity") {
                verbosity = values["verbosity"].as<int>();
            } else if (name=="output") {
                output_file = values["output"].as<string>();
            } else if (name=="csv") {
                csv = true;
            } else if (name=="positional") {
                for (const string &p : values["positional"].as<vector<string>>()) {
                    auto pp = split(p, "=");
                    if (pp.size()==1) {
                        std::ifstream file(p);
                        if (file.fail()) {
                            throw string_exception("cannot open props file '%s'", p);
                        }
                        props_files.push_back(p);
                    } else {
                        props[pp[0]] = pp[1];
                    }
                }
            } else {
                props[name] = as_string(name);
            }
        }
    }
    return result;
}

/************************************************************************
 * as_string - return the named option as a string. This is ugly but there
 * doesn't seem to be a tidy way to convert a boost::any into a string
 * without explicit knowledge of its type. Go figure.
 ***********************************************************************/

string command::as_string(const string &name)
{
    string result = "";
    try {
        result = lexical_cast<string>(values[name].as<int>());
    } catch (...) {
        try {
            result = lexical_cast<string>(values[name].as<float>());
            } catch (...) {
            try {
                result = values[name].as<string>();
            } catch (...) {
                try {
                    result = join(values["positional"].as<vector<string>>(), ",");
                } catch (...) {};
            }
        }
    }
    return result;
}

