import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from dataclasses import dataclass
from geometry import *
import functools
from math import *
import os

susceptible_color = 'greenyellow'
infected_color = 'red'
recovered_color = 'deepskyblue'

default_size = 10
map_size_scale = 300

class raindrop():

    @dataclass
    class _city_info():
        name: str
        location: point
        map_location: point
        population: int
        size: float
        map_size: float = 0
        infected: int = 0
        finished: int = 0
        infected_size: float = 0
        finished_size: float = 0

        def size(self, p):
            return p * self.map_size / self.population

    def __init__(self, world, cities, x_size=None, y_size=None):
        self.world = world
        self.fig = plt.figure(figsize=(x_size or default_size, y_size or default_size))
        #self.ax = self.fig.add_axes([0, 0, 1, 1], frameon=False)
        #self.ax.set_xlim(0, 1)
        #self.ax.set_ylim(0, 1)
        self.cities = {}
        self.min_pop = 0
        self._build_cities(cities)
        self.outlines = plt.scatter(self.city_data(lambda c: c.map_location.x),
                                    self.city_data(lambda c: c.map_location.y),
                                    s=self.city_data(lambda c: c.map_size),
                                    facecolors=susceptible_color)
        self.infected = plt.scatter(self.city_data(lambda c: c.map_location.x),
                                    self.city_data(lambda c: c.map_location.y),
                                    s=self.city_data(lambda c: 0),
                                    facecolors=infected_color,zorder=2)
        self.finished = plt.scatter(self.city_data(lambda c: c.map_location.x),
                                    self.city_data(lambda c: c.map_location.y),
                                    s=self.city_data(lambda c: 0),
                                    facecolors=recovered_color,zorder=3)

    def do_day(self, day):
        daily = self.world.daily[day or 1]
        for c in daily['cities'].values():
            my_c = self.cities[c.name]
            my_c.finished = c.recovered
            my_c.infected = c.infected + my_c.finished
            my_c.finished_size = my_c.map_size * my_c.finished / my_c.population
            my_c.infected_size = my_c.map_size * my_c.infected / my_c.population
        self.infected.set_sizes(self.city_data(lambda c: c.infected_size))
        self.finished.set_sizes(self.city_data(lambda c: c.finished_size))

    def plot(self):
        self.animation = FuncAnimation(self.fig, self.do_day,
                                       frames=range(1, len(self.world.daily) - 1),
                                       repeat=False)
        plt.show()

    def _build_cities(self, cities):
        for c in cities.values():
            self.min_pop = min(self.min_pop or c.population, c.population)
            self.cities[c.name] = raindrop._city_info(
                name=c.name,
                size=c.size,
                location = c.location,
                map_location = self._make_map_location(c.location),
                population=c.population
            )
        for c in self.cities.values():
            c.map_size = self._make_map_size(c.population)

    def city_data(self, fn):
        return [ fn(c) for c in self.cities.values()]

    def _make_map_location(self, loc):
        p = point(loc.x / self.world.size, loc.y / self.world.size)
        return p

    def _make_map_size(self, pop):
        return map_size_scale * pop / self.min_pop

