from geometry import *
import random
import math
from utility import *
from reciprocal import reciprocal
from properties import properties
from constructor import constructor
import itertools

from city import city
from cluster import cluster
from person import person
from infection_counter import infection_counter

DEFAULT_SIZE = 100
GRID_SIZE = 10
DEFAULT_CITY_SHAPE = 1
DEFAULT_POP = 100000
DEFAULT_CITIES = 10
DEFAULT_CITY_MIN_POP = 2000
DEFAULT_CITY_MAX_POP = 20000

class world(infection_counter) :

    arg_table = {
        'population' : (int, DEFAULT_POP),
        'size_x' : (float, DEFAULT_SIZE),
        'size_y' : (float, DEFAULT_SIZE),
        'auto_immunity' : (float, 0.0),
        'travel' : (float,0.01),
        'infectiousness' : (float, 0.01),
        'city_exposure' : (float, 0.001),
        'cluster_exposure' : (float, 0.01),
        'recovery_time' : (int, 7),
    }

    def __init__(self, props=None, cmd_args=None, **kwargs) :
        constructor(world.arg_table, args=kwargs, props=props, cmd_args=cmd_args).apply(self)
        infection_counter.__init__(self)
        self.props = props
        self.geometry = geometry(self.size_x, self.size_y)
        self.city_exposure *= self.infectiousness
        self._add_cities()
        self._add_people()
        cluster.nest_clusters(self)

    def reset(self):
        for c in self.cities :
            c.reset()

    def _add_cities(self) :
        self.city_count = self.props.get(int, 'city', 'count')
        if self.city_count==0 :
            self.city_count = int(sround(int(math.sqrt(self.population) / self.props.get(int, 'city', 'ordinality')), 2))
            self.city_max_pop = int(sround(self.population // 3, 2))
            self.city_min_pop = int(sround((self.population - self.city_max_pop) // \
                                           int(self.city_count * self.props.get(float, 'city', 'min_size_multiplier')), 2))
        else :
            self.city_max_pop, self.city_min_pop = self.props.get(int, 'city', 'mac_pop'), self.props.get(int, 'city', 'min_pop')
        self.cities = []
        pops = reciprocal(self.city_count,self.city_min_pop, self.city_max_pop, self.population).get()
        for i, pop in enumerate(pops) :
            location = self.geometry.random_location()
            c = city(i, location, pop, self)
            self.cities.append(c)
        done = False
        while not done :
            done = True
            for c1, c2 in itertools.combinations(self.cities, 2) :
                if c1.touches(c2) :
                    c2.location = self.geometry.random_location()
                    done = False
        for c in self.cities :
            c.make_neighbors()

    def _add_people(self):
        self.people = []
        print('Adding population ', end='')
        ticks = self.population//50
        for n in range(self.population) :
            p = person(n, self)
            if (n % ticks)==0 :
                print ('.', end='')
            self.people.append(p)

    def get_random_city(self) :
        return get_random_member(self.cities, lambda c: c.pop)

    def get_auto_immunity(self):
        return self.auto_immunity

    def get_city_exposure(self):
        return self.city_exposure

    def get_cluster_exposure(self):
        return self.cluster_exposure

    def get_infectiousness(self):
        return self.infectiousness

    def one_day(self, day: int):
        self.reset()
        for p in self.people :
            if p.is_infected():
                p.infectious(day)
        for p in self.people :
            p.expose(day)


if __name__=='__main__' :
    props = properties('p1.props')
    print(props.dump())
    cluster.make_cluster_info(props)
    w = world(props=props)
    for i in range(10) :
        p = get_random_member(w.people, lambda p: 1)
        print('\n', str(p), 'city:', str(p.city))
        if p.clusters.values() :
            for c in p.clusters.values() :
                print(indent(c.show_detail(), size=2))





