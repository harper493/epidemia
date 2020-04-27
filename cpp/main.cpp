#include "common.h"
#include "world.h"
#include "properties.h"
#include "cluster.h"

int main(int argc, const char **argv)
{
    properties *props = new properties();
    props->add_from_file(argv[1]);
    cluster_type::build(props);
    world *the_world = new world(props);
    the_world->build();
    return 0;
}
