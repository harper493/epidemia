from world import world
import os
import sys
sys.exit(1)
w = world()
print('Total Pop =', w.actual_pop)
for c in w.cities :
    print('%4d %20s %6d %6d' % (c.name, str(c.location), c.pop, c.initial_pop))