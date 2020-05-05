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
    { "10d", "population" },
    { "9d", "infected" },
    { "9d", "total" },
    { "7.2f", "growth" },
    { "9d", "immune" },
    { "10d", "untouched_cities" },
};

world *the_world = NULL;
command *the_args = NULL;

int main(int argc, const char **argv)
{
    the_args = new command();
    try {
        if (!the_args->parse(argc, argv)) {
            return 0;
        }
    } catch (std::exception &exc) {
        std::cerr << exc.what() << std::endl;
        return 1;
    }
    properties *props = new properties();
    for (const string &p : the_args->get_props_files()) {
        props->add_from_file(p);
    }
    for (auto i : the_args->get_props()) {
        props->add_property(i.first, i.second);
    }
    log_output logger(the_args->get_output_file(), the_args->get_csv(), true, log_columns);
    random::initialize(props->get_numeric("random"));
    the_world = new world(props);
    the_world->build();
    the_world->run(logger);
    ptime run_complete_time(microsec_clock::local_time());
    auto build_time = the_world->get_build_complete_time() - the_world->get_start_time();
    auto run_time = run_complete_time - the_world->get_build_complete_time();
    if (the_args->get_verbosity()) {
        std::cout << formatted("\nPopulation %d build time %.3fS run time %.3fS (%d mS/day) %d threads\n",
                               the_world->get_population(),
                               build_time.total_microseconds() / 1e6,
                               run_time.total_microseconds() / 1e6,
                               run_time.total_microseconds() / (1000.0 * (float)the_world->get_day()),
                               the_world->get_thread_count());
    }
    return 0;
}
