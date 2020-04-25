#include "common.h"
#include "world.h"
#include "properties.h"

int main(int argc, const char **argv)
{
    properties *props = new properties();
    props->add_from_file("epidemia-fast.props");
    world *the_world = new world(props);
    return 0;
}
