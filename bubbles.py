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
import time
from constructor import constructor
from plotter import plotter

graph_height = 0.2

susceptible_color = 'deepskyblue'
infected_color = 'red'
recovered_color = 'lime'
button_color = 'linen'
button_hovercolor = 'bisque'

background_color = 'ivory'

graph_total_color = 'red'
graph_infected_color = 'darkorange'

default_size = 10

default_bubble_size = 700
bubble_size_base_world_size = 100

bubble_top = 0.9
bubble_bottom = 0.3

graph_top = 0.3
graph_bottom = 0.1
common_left = 0.15
common_right = 0.85
button_left = 0.8
button_width = 0.1
button_bottom = 0.92
button_height = 0.03

bubble_margin = 5
bubble_text_y_offset = 0.05
label_top = 1
label_y_offset = 0.04
label_left = 1.025

animation_interval = 200           # mS

initial_x_limit = 200
x_increase_delta = 100
initial_y_min = 100

bubble_types = (    # listed from innermost to outermost
    'dead',
    'vaccinated',
    'immune',
    'recovered',
    'gestating',
    'asymptomatic',
    'infected',
)

class bubbles(plotter):

    @dataclass
    class _city_info():
        name: str
        location: point
        map_location: point
        population: int
        size: float
        infected: int = 0
        finished: int = 0
        infected_size: float = 0
        finished_size: float = 0

    arg_table = {
        'population': (int, 0),
        'world_size': (int, 0)
    }

    def __init__(self, **kwargs):
        constructor(bubbles.arg_table, kwargs).apply(self)
        super().__init__(**kwargs)
        self.graph_height = graph_height
        self.right_margin = 0.2
        self.square = True
        self.max_infected = 0
        self.bubble_size = default_bubble_size * bubble_size_base_world_size / self.world_size
        self.lines = (plotter.make_line_info('total', color=self.colors.total, style='solid'),
                      plotter.make_line_info('infected', color=self.colors.infected, style='dashed'),
                      plotter.make_line_info('dead', color=self.colors.dead, style='dotted'))
        self.cities = None

    def build_extra(self):
        self.bubble_chart = self.fig.add_axes([self.left_margin, bubble_bottom,
                                               (1 - self.left_margin - self.right_margin),
                                               bubble_top-bubble_bottom])
        self.bubble_chart.set_xlim(-bubble_margin, self.world_size+bubble_margin)
        self.bubble_chart.set_ylim(-bubble_margin, self.world_size + bubble_margin)
        self.total_box = self.graph.text(label_left, 0.9,
                                         '', color = self.colors.total,
                                         transform = self.graph.transAxes)
        self.infected_box = self.graph.text(label_left, 0.7,
                                            '', color = self.colors.infected,
                                            transform = self.graph.transAxes)
        self.dead_box = self.graph.text(label_left, 0.6,
                                        '', color = self.colors.dead,
                                        transform = self.graph.transAxes)
        self.add_buttons()
        self.top_ax = self.bubble_chart

    def day_pre_extra(self, day, world, daily):
        if self.cities is None :
            self._build_cities(world.cities)
            y = label_top - label_y_offset
            self.scatters = { }
            self.labels = {}
            self.make_one_scatter('susceptible',
                                  lambda c: self.make_bubble_size(c, 'population', ''),
                                  self.colors.susceptible, 0)
            self.add_label('susceptible', y, self.colors.susceptible)
            for z,b in enumerate(bubble_types):
                color = getattr(self.colors, b)
                self.make_one_scatter(b, lambda c: 0, color, len(bubble_types) - z + 1)
                y -= label_y_offset
                self.add_label(b, y, color)

    def add_label(self, b, y, color):
        text = self.bubble_chart.text(label_left, y, b,
                                      color=color,
                                      size=9,
                                      transform=self.bubble_chart.transAxes)
        text.draw(self.bubble_chart.figure.canvas.get_renderer())
        self.labels[b] = text

    def make_one_scatter(self, b, fn, color, z):
        self.scatters[b] = self.bubble_chart.scatter([ c.map_location.x for c in self.cities.values() ],
                                                     [ c.map_location.y for c in self.cities.values() ],
                                                     s=[ fn(c) for c in self.cities.values() ],
                                                     facecolors=color, zorder=z)

    def add_buttons(self):
        if self.display:
            self.replay_button_axes = self.fig.add_axes([button_left, button_bottom, button_width, button_height])
            self.replay_button = Button(self.replay_button_axes, 'Replay',
                                        color=button_color, hovercolor=button_hovercolor)
            self.replay_button.on_clicked(self.replay)

    def replay(self, *args, **kwargs):
        self.reset()
        self.max_infected = 0
        self._make_animation()

    def day_extra(self, day, world, daily):
        self.fig.suptitle(f'\nDay{day:5d}')
        for b, prev_b in zip(bubble_types, ([None] + list(bubble_types))[:-1]):
            self.scatters[b].set_sizes([ self.make_bubble_size(c, b, prev_b) for c in daily.cities.values()])
        for b, text in self.labels.items():
            percent = 100 * getattr(daily, b) / self.population
            text.set_text(f'{b.title()} {percent:4.1f}%')
        total_percent = 100 * daily.total / world.population
        infected_percent = 100 * daily.infected / world.population
        dead_percent = 100 * daily.dead / world.population
        self.max_infected = max(daily.infected, self.max_infected)
        max_infected_percent = 100 * self.max_infected / world.population
        self.total_box.set_text(f'Total {total_percent:4.1f}% ({daily.total})')
        self.infected_box.set_text(f'Infected {infected_percent:4.1f}% ({daily.infected})'
                                   f'\nMax {max_infected_percent:4.1f}% ({self.max_infected})')
        self.dead_box.set_text(f'Dead {dead_percent:4.1f}% ({daily.dead})')

    def make_bubble_size(self, data, b, prev_b):
        pop = getattr(data, b)
        if prev_b and pop > 0:
            inner = getattr(data, prev_b + '_inner')
            total = inner + pop
        else:
            inner, total = 0, pop
        setattr(data, b + '_inner', total)
        result = self.bubble_size * total / self.min_pop
        return result

    def _make_animation(self):
        interval = 1 if self.world.args.no_display else animation_interval
        self.animation = FuncAnimation(self.fig, self.do_day,
                                       frames=bubbles.range_maker(self),
                                       blit=True,
                                       repeat=False,
                                       interval=interval)

    def _build_cities(self, cities):
        self.min_pop = 0
        self.cities = {}
        for c in cities.values():
            self.min_pop = min(self.min_pop or c.population, c.population)
            self.cities[c.index] = bubbles._city_info(
                name=c.name,
                size=c.size,
                location = c.location,
                map_location = self._make_map_location(c.location),
                population=c.population
            )

    def _make_map_location(self, loc):
        p = point(loc.x, loc.y)
        return p


