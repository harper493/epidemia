from cluster import cluster
from geometry import *
from utility import *
import math
from functools import partial

class city(object) :

    class neighbor(object) :

        def __init__(self, city_, dist):
            self.city_, self.dist = city_, dist

    def __init__(self, name, location_: point, pop: int, world_):
        self.name, self.location, self.pop, self.world_ = name, location_, pop, world_
        self.initial_pop = pop
        self.grid = None
        self.exposure = 0
        self._make_size()
        cluster.make_clusters(world_, self)

    def __str__(self):
        return '%3d %16s pop %6d size %.3f nbrs %s' % (self.name, str(self.location), self.pop, self.size,
              ', '.join([ '%s:%.1f' % (n.city_.name, n.dist) for n in self.neighbors ]))

    def reset(self):
        self.exposure = 0

    def get_random_location(self):
        bearing = random.uniform(0, 2*math.pi)
        radius = random.uniform(0,1)**2 * self.size
        return self.location + point(radius * math.sin(bearing), radius * math.cos(bearing))

    def distance(self, c: 'city'):
        return self.location.dist(c.location)

    def pick_clusters(self, location):
        result = {}
        for cname, c in self.clusters.items() :
            result[cname] = get_random_member(c[0], lambda c: c.pop)
        return result

    def make_neighbors(self):
        self.neighbors = []
        for c in self.world_.cities :
            if c is not self :
                self.neighbors.append(city.neighbor(c, self.distance(c)))
        self.neighbors.sort(key=lambda c: c.dist)
        print(str(self))

    def get_random_neighbor(self):
        return get_random_member(self.neighbors, lambda n: 1/n.dist)

    def touches(self, other: 'city'):
        return self.size + other.size > self.distance(other)

    def _make_size(self):
        minp = self.world_.props.get(int, 'city_min_pop')
        maxp = self.world_.props.get(int, 'city_max_pop')
        mind = self.world_.props.get(int, 'city', 'min_density')
        maxd = self.world_.props.get(int, 'city', 'max_density')
        z1 = self.pop - minp
        z2 = maxp - minp
        z3 = z1/z2
        z4 = maxd - mind
        z5 = z3*z4
        z6 = z5 + mind
        z = ((self.pop-minp) / (maxp-minp)) * (maxd-mind) + mind
        area = self.pop /z6
        z8 = math.sqrt(area / math.pi)
        self.size = z8

