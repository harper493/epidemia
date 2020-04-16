from geometry import *
import random
from utility import *
from reciprocal import reciprocal
from properties import properties
from constructor import constructor
import itertools
from math import *
from copy import copy
import time

from city import city
from cluster import cluster
from person import person
from infection_counter import infection_counter
from cached_choice import cached_choice

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
        'initial_infected' : (int, 20),
    }

    def __init__(self, props=None, cmd_args=None, **kwargs) :
        start_time = time.time()
        constructor(world.arg_table, args=kwargs, props=props, cmd_args=cmd_args).apply(self)
        infection_counter.__init__(self)
        self.day = self.next_day = 0
        self.prev_infected = self.initial_infected
        self.total_infected = self.initial_infected
        self.prev_total = self.initial_infected
        self.max_infected = 0
        self.immune = 0
        self.growth = self.max_growth = 1
        self.highest_day = 0
        self.days_to_double = 0
        self.daily = {}
        self.props = props
        self.geometry = geometry(self.size_x, self.size_y)
        self.city_exposure *= self.infectiousness
        self._add_cities()
        self.city_cache = cached_choice(self.cities, lambda c: c.pop)
        self._add_people()
        self.infected_list = []
        self.susceptible_list = copy(self.people)
        cluster.nest_clusters(self)
        self.new_infected_list = []
        for p in random.choices(self.people, k=self.initial_infected):
            p.infect(0)
        self.infected_list = self.new_infected_list
        self.setup_time = time.time() - start_time
        self.start_time = time.time()

    def reset(self):
        for c in self.cities :
            c.reset()

    def _add_cities(self) :
        self.city_count = self.props.get(int, 'city', 'count')
        if self.city_count==0 :
            self.city_count = max(int(self.props.get(int, 'city', 'min_count')),
                    int(sround(int(pow(self.population, self.props.get(float, 'city', 'auto_power'))
                                                 / self.props.get(int, 'city', 'auto_divider')), 2)))
            self.city_max_pop = int(sround(self.population // 3, 2))
            self.city_min_pop = int(sround((self.population - self.city_max_pop) // \
                                           int(self.city_count * self.props.get(float, 'city', 'min_size_multiplier')), 2))
            print(f'!!! {self.city_count=} {self.city_max_pop=} {self.city_min_pop=}')
        else :
            self.city_max_pop, self.city_min_pop = self.props.get(int, 'city', 'max_pop'), self.props.get(int, 'city', 'min_pop')
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
        return self.city_cache.choose()

    def get_auto_immunity(self):
        return self.auto_immunity

    def get_city_exposure(self):
        return self.city_exposure

    def get_cluster_exposure(self):
        return self.cluster_exposure

    def get_infectiousness(self):
        return self.infectiousness

    def infect_one(self, p):
        was_infected = p.is_infected()
        infection_counter.infect_one(self, p)
        if not was_infected :
            self.new_infected_list.append(p)

    def one_day(self):
        self.new_infected_list = []
        self.new_susceptible_list = []
        self.prev_infected = self.infected
        self.prev_total = self.total_infected
        self.day = self.next_day
        self.next_day += 1
        self.reset()
        for p in self.infected_list :
            p.infectious(self.day)
        for p in self.susceptible_list :
            p.expose(self.day)
            if p.is_susceptible() :
                self.new_susceptible_list.append(p)
        self.total_infected = self.infected + self.recovered - self.never_infected
        self.immune = self.recovered - self.never_infected
        if self.infected > self.max_infected :
            self.max_infected = self.infected
            self.highest_day = self.day
        self.growth = self.infected / self.prev_infected
        if self.infected > self.population//100 and self.growth > self.max_growth:
            self.max_growth = self.growth
        if self.max_growth > 1 :
            self.days_to_double = log(2) / log(self.max_growth)
        self.run_time = time.time() - self.start_time
        self.daily[self.day] = make_dict(self, 'day', 'infected', 'total_infected', 'recovered', 'immune',
                                               'growth')
        self.infected_list = [ p for p in self.infected_list if p.is_infected()] + self.new_infected_list
        self.susceptible_list = self.new_susceptible_list
        return self.infected >= self.prev_infected or self.infected > self.population // 1000

    def get_days(self):
        return [ k for k in self.daily.keys() ]

    def get_data(self, name):
        return [ d[name] for d in self.daily.values() ]

    def get_data_point(self, name, day):
        return self.daily[day][name]

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





