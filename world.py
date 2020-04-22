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
from lognormal import lognormal

DEFAULT_SIZE = 100
GRID_SIZE = 10
DEFAULT_CITY_SHAPE = 1
DEFAULT_POP = 100000
DEFAULT_CITIES = 10
DEFAULT_CITY_MIN_POP = 2000
DEFAULT_CITY_MAX_POP = 20000

class people_list() :

    def __init__(self, content=None):
        self.current = set()
        self.new = set()
        if content :
            for c in content :
                self.new.add(c)

    def reset(self):
        self.current = self.new
        self.new = set()

    def add(self, p):
        self.new.add(p)

    def __iter__(self):
        for p in self.current:
            yield p

    def validate(self, pred):
        for p in self.current :
            if not pred(p) :
                a = 1

class world(infection_counter) :

    arg_table = {
        'population' : (int, DEFAULT_POP),
        'city_max_pop' : (int, 0),
        'size_x' : (float, DEFAULT_SIZE),
        'size_y' : (float, DEFAULT_SIZE),
        'auto_immunity' : (float, 0.0),
        'travel' : (float,0.01),
        'infectiousness' : (float, 2),
Add city exposure, scaled by city size        'cluster_exposure' : (float, 0.01),
        'recovery_time' : (int, 7),
        'initial_infected' : (int, 20),
        'infected_cities' : (float, 0.5)
    }

    def __init__(self, props=None, cmd_args=None, **kwargs) :
        start_time = time.time()
        constructor(world.arg_table, args=kwargs, props=props, cmd_args=cmd_args).apply(self)
        infection_counter.__init__(self)
        self.recovery_dist = lognormal(props.get(float, 'recovery_time'), props.get(float, 'recovery_sd'))
        self.gestating_dist = lognormal(props.get(float, 'gestating_time'), props.get(float, 'gestating_sd'))
        self.day = self.next_day = 0
        self.pop = self.population               # backward compatibility hack
        self.prev_infected = self.initial_infected
        self.total_infected = self.initial_infected
        self.prev_total = self.initial_infected
        self.prev_recovered = 0
        self.max_infected = 0
        self.immune = 0
        self.growth = self.max_growth = 1
        self.days_to_double = 0
        self.daily = {}
        self.props = props
        self.geometry = geometry(self.size_x, self.size_y)
        self._make_infection_prob()
        self._add_cities()
        self.untouched_cities = len(self.cities)
        self.untouched_clusters = sum([c.get_untouched_clusters() for c in self.cities])
        self.susceptible_clusters = sum([c.get_susceptible_clusters() for c in self.cities])
        self.city_cache = cached_choice(self.cities, lambda c: c.target_pop)
        cluster.nest_clusters(self)
        self.infected_list = people_list()
        self.gestating_list = people_list()
        self._add_people()
        self.total_clusters = sum([c.get_leaf_clusters() for c in self.cities])
        self.cities_by_pop = sorted(self.cities, key=lambda c: c.pop, reverse=True)
        for c in self.cities :
            c.set_exposure()
        self._infect_cities()
        self.susceptible_list = people_list([ p for p in self.people if p.is_susceptible() ])
        self.setup_time = time.time() - start_time
        self.start_time = time.time()

    def reset(self):
        for c in self.cities :
            c.reset()

    def _add_cities(self) :
        self.city_count = self.props.get(int, 'city_count')
        if self.city_count==0 or self.city_max_pop==0 :
            self.city_count = self.city_count or max(int(self.props.get(int, 'city', 'min_count')),
                                int(sround(int(pow(self.pop, self.props.get(float, 'city', 'auto_power'))
                                                 / self.props.get(int, 'city', 'auto_divider')), 2)))
            self.city_max_pop = int(sround(self.pop // 3, 2))
            self.city_min_pop = int(sround((self.pop - self.city_max_pop) // \
                                           int(self.city_count * self.props.get(float, 'city', 'min_size_multiplier')), 2))
            #print(f'!!! {self.city_count=} {self.city_max_pop=} {self.city_min_pop=}')
        else :
            self.city_max_pop, self.city_min_pop = self.props.get(int, 'city', 'max_pop'), self.props.get(int, 'city', 'min_pop')
        self.cities = []
        pops = reciprocal(self.city_count,self.city_min_pop, self.city_max_pop, self.pop).get()
        for i, pop in enumerate(pops) :
            location = self.geometry.random_location()
            c = city(f'C{len(self.cities)}', location, pop, self)
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
        for n in range(self.pop) :
            p = person(f'P{len(self.people)}', self)
            self.people.append(p)
            p.city.add_person(p)

    def _infect_cities(self):
        if self.infected_cities==0 :
            city_count = len(self.cities)
        elif self.infected_cities<1 :
            city_count = int(len(self.cities) * self.infected_cities)
        else :
            city_count = int(self.infected_cities)
        if self.infected_cities==len(self.cities) :
            inf_cities = copy(self.cities)
        else :
            inf_cities = []
            remaining = copy(self.cities)
            while len(inf_cities) < city_count :
                cc = cached_choice(remaining, lambda c: sqrt(c.target_pop))
                c = cc.choose()
                inf_cities.append(c)
                del remaining[remaining.index(c)]
        count = len(inf_cities)
        for c in inf_cities :
            self._infect_one_person(c)
        cc = cached_choice(inf_cities, lambda c: c.target_pop)
        while count < self.initial_infected :
            self._infect_one_person(cc.choose())
            count += 1

    def _infect_one_person(self, city):
        while True :
            p = random.choices(city.people)[0]
            if p.is_susceptible():
                p.infect(0)
                self.infected_list.add(p)
                break

    def _make_infection_prob(self):
        exposure_time = self.props.get(int, 'recovery_time') - self.props.get(int, 'gestating_time')
        cluster_factor = sum([ cl['rms'] for cl in cluster.cluster_info.values() ])
        self.infection_prob = self.infectiousness / (exposure_time * cluster_factor)

    def get_random_city(self) :
        return self.city_cache.choose()

    def get_biggest_city(self):
        return self.cities_by_pop[0]

    def get_smallest_city(self):
        return self.cities_by_pop[-1]

    def get_auto_immunity(self):
        return self.auto_immunity

    def get_cluster_exposure(self):
        return self.cluster_exposure

    def get_infectiousness(self):
        return self.infectiousness

    def get_infection_prob(self):
        return self.infection_prob

    def one_day(self):
        self.infected_list.reset()
        self.susceptible_list.reset()
        self.gestating_list.reset()
        self.prev_infected = self.infected
        self.prev_recovered = self.recovered
        self.prev_total = self.total_infected
        self.day = self.next_day
        self.next_day += 1
        self.reset()
        for p in self.infected_list :
            p.infectious(self.day)
            if p.is_infected() :
                self.infected_list.add(p)
        for p in self.gestating_list :
            if p.gestating(self.day) :
                assert(p.is_infected())
                self.infected_list.add(p)
            else :
                self.gestating_list.add(p)
        for p in self.susceptible_list :
            p.expose(self.day)
            if p.is_susceptible() :
                self.susceptible_list.add(p)
            elif p.is_gestating() :
                self.gestating_list.add(p)
        self.total_infected = self.infected + self.recovered
        if self.infected > self.max_infected :
            self.max_infected = self.infected
            self.highest_day = self.day
        self.growth = self.infected / (self.prev_infected or 1)
        if self.infected > self.pop//100 and self.growth > self.max_growth:
            self.max_growth = self.growth
        if self.max_growth > 1 :
            self.days_to_double = log(2) / log(self.max_growth)
        self.untouched_cities = sum([1 for c in self.cities if c.is_untouched()])
        self.untouched_clusters = sum([c.get_untouched_clusters() for c in self.cities])
        self.susceptible_clusters = sum([c.get_susceptible_clusters() for c in self.cities])
        self.run_time = time.time() - self.start_time
        self.daily[self.day] = make_dict(self, 'day', 'infected', 'total_infected', 'recovered', 'immune',
                                               'growth')
        return self.infected >= self.prev_infected \
               or (self.infected > self.pop // 1000) \
               or self.day < 20

    def run(self, pred=None, logger=None):
        good = True
        while good :
            good = self.one_day()
            if logger :
                logger(self)
            if pred :
                good = pred(self)

    def get_days(self):
        return [ k for k in self.daily.keys() ]

    def get_data(self, name):
        return [ d[name] for d in self.daily.values() ]

    def get_data_point(self, name, day):
        return self.daily[day][name]

    def get_interesting(self):
        from_ = None
        to = len(self.daily)
        for d in self.daily.keys():
            ti = self.get_data_point('total_infected', d)
            if from_:
                if d > self.highest_day and self.get_data_point('infected', d) < ti // 5:
                    to = d
                    break
            elif ti > sqrt(self.population):
                from_ = d
        return(from_ or 0, to)


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
