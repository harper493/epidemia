import matplotlib.pyplot as plt
import os
from matplotlib import animation
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
import numpy as np
from utility import *
from dynamic_plot import dynamic_plot
import time

default_x_size = 12
default_y_size = 8
initial_x_limit = 200
x_increase_delta = 100
initial_y_min = 100

class plotter(dynamic_plot):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    class range_maker():

        def __init__(self, worlds):
            self.worlds = worlds
            self.world_no = 0
            self.highest_day = 0

        def __iter__(self):
            while True:
                daily = None
                while True:
                    w = self.worlds[self.world_no]
                    if w:
                        d = w.get_daily(self.highest_day + 1)
                        if d:
                            daily = d
                            self.highest_day += 1
                        elif daily:
                            break
                        else:
                            time.sleep(0.1)
                if daily.infected < 0:
                    self.world_no += 1
                    self.highest_day = 0
                    if self.world_no >= len(self.worlds):
                        return
                yield (self.world_no, self.highest_day)

    def plot(self, x, *data):
        self.pre_plot(x_size=default_x_size, y_size=default_y_size)
        for d in data:
            style = d.get('style', '-')
            color = d.get('color', None)
            label = d.get('label', None)
            plt.plot(x, d['data'], label=label, linestyle=style, color=color)
        plt.xlabel('Days')
        plt.ylabel('People')
        if self.title :
            plt.title(self.title, fontsize=10)
        if self.log_scale :
            plt.yscale('log')
        if self.legend:
            plt.legend(loc='upper left')
        if False and self.file :
            self.format = self.format or 'png'
            if '.' not in os.path.basename(self.file) :
                file = f'{self.file}.{format}'
            plt.savefig(file, bbox_inches='tight', format=self.format or 'png')
        plt.show()

    def plot_dynamic(self, worlds, labels):
        self.pre_plot(x_size=default_x_size, y_size=default_y_size)
        self.worlds = worlds
        self.labels = labels
        self.last_day = 0
        self.last_world = 0
        self.build()
        self.animation = FuncAnimation(self.fig, self.do_day,
                                       frames=plotter.range_maker(self.worlds),
                                       blit=False,
                                       repeat=False)
        plt.show()

    def build(self):
        self.graph = self.fig.add_axes((0.1, 0.1, 0.8, 0.8))
        self.x_values = [ d for d in range(1, initial_x_limit) ]
        self.graph.set_xlim(0, len(self.x_values))
        self.graph.set_xlabel('Days')
        self.graph.set_ylabel('People')
        self.values, self.graph_lines = [], []
        if self.title :
            self.fig.suptitle(self.title, fontsize=10)
        if self.log_scale :
            self.graph.set_yscale('log')
        for lab, color in zip(self.labels, self.colors.graph):
            self.values.append({ l.element : [np.nan] * len(self.x_values) for l in self.lines})
            self.graph_lines.append({ l.element : self.graph.plot(self.x_values, self.values[-1][l.element],
                                                                  color=l.color or color,
                                                                  label=lab if l.style=='solid' else None,
                                                                  linestyle=l.style or 'solid')[0]
                                      for l in self.lines})
        if self.legend:
            self.graph.legend(loc='upper left')
        self.first = True

    def do_day(self, r):
        world_no, day = r
        if world_no != self.last_world:
            self.last_world = world_no
            self.last_day = 0
        while self.last_day < day:
            self.last_day += 1
            if self.last_day > len(self.x_values) :
                self.extend_x()
            w = self.worlds[world_no]
            daily = w.get_daily(self.last_day)
            if self.first:
                self.first = False
                self.graph.set_ylim(initial_y_min, w.population)
            for l in self.lines:
                e = l.element
                self.values[world_no][e][self.last_day-1] = getattr(daily, e)
                self.graph_lines[world_no][e].set_ydata(self.values[world_no][e])

    def extend_x(self):
        old_x = len(self.x_values)
        new_x = old_x + x_increase_delta
        self.x_values += [ d for d in range(old_x, new_x) ]
        self.graph.set_xlim(0, len(self.x_values))
        for w in range(len(self.labels)):
            for l in self.lines:
                e = l.element
                self.values[w][e] += [ np.nan ] * x_increase_delta
                self.graph_lines[w][e].set_xdata(self.x_values)
                self.graph_lines[w][e].set_ydata(self.values[w][e])


