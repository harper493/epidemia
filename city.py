from cluster import cluster
from infection_counter import infection_counter
from geometry import *
from utility import *
import math
import random
from functools import partial

from cached_choice import cached_choice

class city(infection_counter) :

    class neighbor(object) :

        def __init__(self, city_, dist):
            self.city_, self.dist = city_, dist

    def __init__(self, name, location_: point, pop: int, world_):
        self.name, self.location, self.target_pop, self.world_ = name, location_, pop, world_
        infection_counter.__init__(self, world_)
        self.grid = None
        self.exposure = 0
        self.pop = 0
        self.people = []
        self.exposure = 0
        self.exposure_per_person = None
        self._make_size()
        cluster.make_clusters(world_, self)
        self.cluster_count = None

    def __str__(self):
        return '%3d %16s pop %6d size %.3f nbrs %s' % (self.name, str(self.location), self.pop, self.size,
              ', '.join([ '%s:%.1f' % (n.city_.name, n.dist) for n in self.neighbors ]))

    def reset(self):
        self.exposure = 0
        for cl in self.iter_clusters() :
            cl.reset()

    def get_random_location(self):
        bearing = random.uniform(0, 2*math.pi)
        radius = random.uniform(0,1)**2 * self.size
        return self.location + point(radius * math.sin(bearing), radius * math.cos(bearing))

    def add_person(self, p):
        self.pop += 1
        self.people.append(p)

    def distance(self, c: 'city'):
        return self.location.dist(c.location)

    def pick_clusters(self, location):
        result = {}
        for cname, c in self.clusters.items() :
            result[cname] = c[0].choose()
        return result

    def make_neighbors(self):
        self.neighbors = []
        for c in self.world_.cities :
            if c is not self :
                self.neighbors.append(city.neighbor(c, self.distance(c)))
        self.neighbors.sort(key=lambda c: c.dist)

    def get_random_neighbor(self):
        return get_random_member(self.neighbors, lambda n: 1/n.dist)

    def get_random_person(self):
        return random.choices(self.people)

    def touches(self, other: 'city'):
        return self.size + other.size > self.distance(other)

    def set_exposure(self):
        pr = self.pop / self.world_.get_smallest_city().pop
        self.pop_ratio =  1/ (pr ** self.world_.props.get(float, 'city', 'pop_ratio_power'))
        self.exposure_per_person = self.world_.get_infection_prob() * \
                                   self.world_.props.get(float, 'city', 'exposure') * self.pop_ratio

    def expose(self):
        self.exposure = add_probability(self.exposure, self.exposure_per_person)

    def get_exposure(self):
        return self.exposure * self.pop_ratio

    def get_leaf_clusters(self):
        if self.cluster_count is None :
            self.cluster_count = count(self.iter_leaf_clusters(), lambda cl: cl.pop>0)
        return self.cluster_count

    def get_untouched_clusters(self):
        return count(self.iter_leaf_clusters(), lambda cl: cl.pop>0 and cl.is_untouched())

    def get_susceptible_clusters(self):
        return count(self.iter_leaf_clusters(), lambda cl: cl.pop>0 and cl.is_susceptible())

    def iter_clusters(self):
        for c1 in self.clusters.values() :
            for c2 in c1.values() :
                for c3 in c2 :
                    yield c3

    def iter_leaf_clusters(self):
        for c1 in self.clusters.values() :
            for c2 in c1[0] :
                yield c2

    def _make_size(self):
        minp = self.world_.city_min_pop
        maxp = self.world_.city_max_pop
        mind = self.world_.props.get(int, 'city', 'min_density')
        maxd = self.world_.props.get(int, 'city', 'max_density')
        z1 = self.target_pop - minp
        z2 = maxp - minp
        z3 = z1/z2
        z4 = maxd - mind
        z5 = z3*z4
        z6 = z5 + mind
        z = ((self.target_pop-minp) / (maxp-minp)) * (maxd-mind) + mind
        area = self.target_pop /z6
        z8 = math.sqrt(area / math.pi)
        self.size = z8

