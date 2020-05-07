import matplotlib.pyplot as plt
from matplotlib import animation
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
from dataclasses import dataclass
from geometry import *
import numpy as np
from copy import copy
import functools
from math import *
import os

susceptible_color = 'deepskyblue'
infected_color = 'red'
recovered_color = 'lime'

button_color = 'linen'
button_hovercolor = 'bisque'

background_color = 'ivory'

graph_total_color = 'red'
graph_infected_color = 'darkorange'

default_size = 10
map_size_scale = 300

bubble_top = 0.9
bubble_bottom = 0.35
graph_top = 0.3
graph_bottom = 0.1
common_left = 0.15
common_right = 0.85
button_left = 0.8
button_width = 0.1
button_bottom = 0.92
button_height = 0.03

bubble_margin = 5

class bubbles():

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
        self.fig.patch.set_facecolor(background_color)
        self.bubble_chart = self.fig.add_axes([common_left, bubble_bottom,
                                               common_right-common_left, bubble_top-bubble_bottom])
        self.graph = self.fig.add_axes([common_left, graph_bottom,
                                               common_right-common_left, graph_top-graph_bottom])
        self.cities = {}
        self.title = ''
        self.min_pop = 0
        self._build_cities(cities)
        self.bubble_chart.set_xlim(-bubble_margin, self.world.size+bubble_margin)
        self.bubble_chart.set_ylim(-bubble_margin, self.world.size + bubble_margin)
        self.outlines = self.bubble_chart.scatter(self.city_data(lambda c: c.map_location.x),
                                                  self.city_data(lambda c: c.map_location.y),
                                                  s=self.city_data(lambda c: c.map_size),
                                                  facecolors=susceptible_color)
        self.infected = self.bubble_chart.scatter(self.city_data(lambda c: c.map_location.x),
                                                  self.city_data(lambda c: c.map_location.y),
                                                  s=self.city_data(lambda c: 0),
                                                  facecolors=infected_color, zorder=2)
        self.finished = self.bubble_chart.scatter(self.city_data(lambda c: c.map_location.x),
                                                  self.city_data(lambda c: c.map_location.y),
                                                  s=self.city_data(lambda c: 0),
                                                  facecolors=recovered_color, zorder=3)
        self.x_values = [ d for d in range(1, len(self.world.daily)) ]
        self.total_y = [ np.nan ] * len(self.x_values)
        self.infected_y = [ np.nan ] * len(self.x_values)
        self.total_line, = self.graph.plot(self.x_values, self.total_y, color=graph_total_color)
        self.infected_line, = self.graph.plot(self.x_values, self.infected_y, color=graph_infected_color,
                                              linestyle='--')
        self.graph.set_xlim(0, len(self.world.daily))
        self.graph.set_xlabel('Days')
        self.graph.set_ylabel('People')
        ymin = min([ d['infected'] for d in self.world.daily.values() if d['infected']>0])
        self.graph.set_ylim(ymin, self.world.population)
        self.graph.set_yscale('log')
        self.total_box = self.graph.text(len(self.world.daily) * 1.02,
                                         self.world.population*0.6,
                                         '', color=graph_total_color)
        self.infected_box = self.graph.text(len(self.world.daily) * 1.02,
                                         self.world.population*0.1,
                                         '', color=graph_infected_color)
        self.max_infected = 0
        self.add_button()

    def add_button(self):
        self.replay_button_axes = self.fig.add_axes([button_left, button_bottom, button_width, button_height])
        self.replay_button = Button(self.replay_button_axes, 'Replay',
                                    color=button_color, hovercolor=button_hovercolor)
        self.replay_button.on_clicked(self.replay)

    def replay(self, *args, **kwargs):
        self.total_y = [ np.nan ] * len(self.x_values)
        self.infected_y = [ np.nan ] * len(self.x_values)
        self.max_infected = 0
        self._make_animation()

    def do_day(self, day):
        day = day or 1
        daily = self.world.daily[day]
        self.fig.suptitle(f'\nDay{day:4d}')
        for c in daily['cities'].values():
            my_c = self.cities[c.name]
            my_c.finished = c.recovered
            my_c.infected = c.infected + my_c.finished
            my_c.finished_size = my_c.map_size * my_c.finished / my_c.population
            my_c.infected_size = my_c.map_size * my_c.infected / my_c.population
        self.infected.set_sizes(self.city_data(lambda c: c.infected_size))
        self.finished.set_sizes(self.city_data(lambda c: c.finished_size))
        total_percent = 100 * daily["total_infected"]/self.world.population
        infected_percent = 100 * daily["infected"]/self.world.population
        self.max_infected = max(infected_percent, self.max_infected)
        self.total_box.set_text(f'Total {total_percent:4.1f}%')
        self.infected_box.set_text(f'Infected {infected_percent:4.1f}%\nMax {self.max_infected:4.1f}%')
        self._update_bubble_labels(daily)
        if day==1:
            self.total_y = [np.nan] * len(self.x_values)
            self.infected_y = [np.nan] * len(self.x_values)
        else:
            self.total_y[day - 1] = daily['total_infected']
            self.infected_y[day - 1] = daily['infected']
            self.total_line.set_ydata([ self.total_y ])
            self.infected_line.set_ydata([ self.infected_y ])

    def plot(self, file=None, title='', format=None):
        self.title = title
        self.format = format
        self.file = file
        descr = '\n'.join([ t for t in re.split(r'(.*?\s+[0-9.]+)\s', title) if len(t) ])
        self.description = self.bubble_chart.text(-bubble_margin, self.world.size + bubble_margin * 1.5, descr, size=10)
        self._make_bubble_labels()
        self._make_animation()
        self._save_file()
        plt.show()

    def _make_bubble_labels(self):
        y = self.world.size
        self.bubble_labels = {}
        for n in ('susceptible', 'infected', 'recovered'):
            color = globals()[n+'_color']
            text = self.bubble_chart.text(self.world.size + bubble_margin*1.5, y, n, color=color, size=9)
            text.draw(self.bubble_chart.figure.canvas.get_renderer())
            self.bubble_labels[n] = text
            y -= text.get_window_extent().height / 3

    def _update_bubble_labels(self, daily):
        self._update_one_bubble_label('infected', daily['infected'])
        self._update_one_bubble_label('susceptible', self.world.population - daily['total_infected'] - daily['immune'])
        self._update_one_bubble_label('recovered', daily['total_infected'] + daily['immune'])

    def _update_one_bubble_label(self, name, value):
        text = self.bubble_labels[name]
        percent = 100 * value / self.world.population
        text.set_text(f'{name.title()} {percent:4.1f}%')

    def _make_animation(self):
        self.animation = FuncAnimation(self.fig, self.do_day,
                                       frames=range(1, len(self.world.daily) - 1),
                                       repeat=False,
                                       repeat_delay=5000)

    def _save_file(self):
        if self.file:
            self.format = self.format or 'gif'
            if '.' not in os.path.basename(self.file) :
                self.file = f'{self.file}.{self.format}'
            if self.format == 'mp4':
                Writer = animation.writers['ffmpeg']
                writer = Writer(fps=15)
                self.animation.save(self.file, writer=writer)
            elif self.format == 'gif':
                self.animation.save(self.file, writer='imagemagick', fps=10)
            elif self.format == 'html5':
                with open(self.file, 'w') as f:
                    f.write(self.animation.to_html5_video())

    def _build_cities(self, cities):
        for c in cities.values():
            self.min_pop = min(self.min_pop or c.population, c.population)
            self.cities[c.name] = bubbles._city_info(
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
        p = point(loc.x, loc.y)
        return p

    def _make_map_size(self, pop):
        return map_size_scale * pop / self.min_pop

