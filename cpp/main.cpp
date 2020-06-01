#include "common.h"
#include "world.h"
#include "properties.h"
#include "cluster.h"
#include "formatted.h"
#include "command.h"
#include "log_output.h"
#include "random.h"
#include "city.h"
#include "unit_test.h"

using boost::posix_time::microsec_clock;

log_output::column_defs log_columns{
    { "4d", "day" },
    { "6d", "city" },
    { "8d", "gestating" },
    { "8d", "asymptomatic" },
    { "8d", "infected" },
    { "8d", "total" },
    { "7.2f", "growth" },
    { "8d", "immune" },
    { "8d", "vaccinated" },
    { "8d", "recovered" },
    { "8d", "dead" },
    { "6d", "untouched_cities" },
    { "5.2f", "days_to_double" },
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
    random::initialize(props->get_numeric("random"));
    unit_test::apply();
    the_world = new world(props);
    the_world->build();
    if (!props->get("city_data").empty()) {
        string city_file = props->get("city_data");
        log_output city_logger(city_file, the_args->get_csv(), true, city::get_columns());
        the_world->show_cities(city_logger);
    }
    log_output logger(the_args->get_output_file(), the_args->get_csv(), true, log_columns);
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
