#include "common.h"
#include "world.h"
#include "properties.h"
#include "cluster.h"
#include "formatted.h"

using boost::posix_time::microsec_clock;

int main(int argc, const char **argv)
{
    properties *props = new properties();
    string props_path = argc>1 ? argv[1] : "../base.props";
    if (!props->add_from_file(props_path)) {
        std::cerr << formatted("error reading properties file '%s'\n", props_path);
        return 1;
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
