#include "common.h"
#include "world.h"
#include "properties.h"
#include "cluster.h"
#include "formatted.h"
#include "command.h"
#include "log_output.h"
#include "random.h"

using boost::posix_time::microsec_clock;

log_output::column_defs log_columns{
    { "4d", "day" },
    { "6d", "city" },
    { "9d", "infected" },
    { "8d", "total" },
    { "7.2f", "growth" },
    { "8d", "immune" },
    { "10d", "untouched" },
};

world *the_world = NULL;

int main(int argc, const char **argv)
{
    command args;
    try {
        if (!args.parse(argc, argv)) {
            return 0;
        }
    } catch (std::exception &exc) {
        std::cerr << exc.what() << std::endl;
        return 1;
    }
    properties *props = new properties();
    for (const string &p : args.get_props_files()) {
        props->add_from_file(p);
    }
    for (auto i : args.get_props()) {
        props->add_property(i.first, i.second);
    }
    log_output logger(args.get_output_file(), args.get_csv(), true, log_columns);
    random::initialize(props->get_numeric("random"));
    the_world = new world(props);
    the_world->build();
    the_world->run(logger);
    ptime run_complete_time(microsec_clock::local_time());
    auto build_time = the_world->get_build_complete_time() - the_world->get_start_time();
    auto run_time = run_complete_time - the_world->get_build_complete_time();
    if (args.get_verbosity()) {
        std::cout << formatted("\nPopulation %d build time %.3fS run time %.3fS (%d mS/day)\n",
                               the_world->get_population(),
                               build_time.total_microseconds() / 1e6,
                               run_time.total_microseconds() / 1e6,
                               run_time.total_microseconds() / (1000.0 * (float)the_world->get_day()));
    }
    return 0;
}
