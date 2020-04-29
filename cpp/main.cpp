#include "common.h"
#include "world.h"
#include "properties.h"
#include "cluster.h"
#include "formatted.h"
#include "command.h"

using boost::posix_time::microsec_clock;

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
    ptime start_time(microsec_clock::local_time());
    world *the_world = new world(props);
    the_world->build();
    ptime build_time(microsec_clock::local_time());
    the_world->run();
    ptime run_time(microsec_clock::local_time());
    std::cout << formatted("\nBuild time %.3fS run time %.3fS (%d mS/day)\n",
                           (build_time-start_time).total_microseconds() / 1e6,
                           (run_time-build_time).total_microseconds() / 1e6,
                           (run_time-build_time).total_microseconds() / (1000.0 * (float)the_world->get_day()));
                           
    return 0;
}
