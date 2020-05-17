import matplotlib.pyplot as plt
import os
from matplotlib import animation
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
import numpy as np
from utility import *
from dataclasses import dataclass
import time
from utility import *
from constructor import constructor

default_x_size = 12
default_y_size = 8
initial_x_limit = 200
x_increase_delta = 100
initial_y_min = 100

@dataclass
class line_info():
    element: str
    color: str = None
    style: str = None

class plotter():

    class range_maker():

        def __init__(self, plotter):
            self.plotter = plotter
            self.worlds = plotter.worlds
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
                            if self.plotter.incremental:
                                break
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

    @dataclass
    class colors():
        background = 'ivory'
        button = 'linen'
        button_hover = 'bisque'
        susceptible = 'deepskyblue'
        infected = 'red'
        recovered = 'mediumseagreen'
        immune = 'limegreen'
        vaccinated = 'chartreuse'
        dead = 'black'
        total = 'cornflowerblue'
        graph_infected = 'darkorange'
        graph_total = 'red'
        graph = 'blue,forestgreen,red,cyan,darkviolet,gold,' \
                'sienna,cornflowerblue,lime,lightcoral,turquoise,orange,magenta,' \
                'purple,yellowgreen,rosybrown,steelblue,crimson,black'

        def load(self, props):
            for pname, pvalue in props:
                if pname[0] == 'color':
                    setattr(self, pname[-1], pvalue)
            self.graph = self.graph.split(',')

    arg_table = {
        'title' : (str, ''),
        'log_scale' : (bool, True),
        'legend': (bool, True),
        'file': (str, ''),
        'show': (bool, False),
        'format': (str, ''),
        'props': None,
        'total_color': None,
        'infected_color': None,
        'lines': None,
        'incremental': False,
        'size': None,
        'x_size': default_x_size,
        'y_size': default_y_size,
        'left_margin': 0.06,
        'right_margin': 0.06,
        'bottom_margin': 0.07,
        'graph_height': 0.8,
        'surtitle_left_margin': 0.1,
        'surtitle_base': 1.02,
        'surtitle_line_height': 0.025,
        'surtitle_width': 0.08,
        'surtitle_font_size': 9,
        'display': True,
        'square': False,
        'surtitle': [],    # must be a list of 2-tuples (label, value)
    }

    def __init__(self, **kwargs):
        constructor(plotter.arg_table, args=kwargs).apply(self)
        if self.lines is None:
            self.lines = (line_info('total', style='solid'),
                          line_info('infected', style='dashed'),
                          line_info('dead', style='dotted'))
        self.colors = plotter.colors()
        self.colors.load(self.props)

    @staticmethod
    def make_line_info(*args, **kwargs):
        return line_info(*args, **kwargs)

    def plot(self, worlds, labels):
        self.pre_plot(x_size=default_x_size, y_size=default_y_size)
        self.worlds = worlds
        self.labels = labels
        self.last_day = 0
        self.last_world = 0
        self.build()
        self.animation = FuncAnimation(self.fig, self.do_day,
                                       frames=plotter.range_maker(self),
                                       blit=False,
                                       repeat=False)
        plt.show()

    def build(self):
        size = self.props.get(int, 'plot', 'size') or self.size
        x_size = self.props.get(int, 'plot', 'x_size') or self.size or self.x_size
        if self.square:
            y_size = x_size
        else:
            y_size = self.props.get(int, 'plot', 'y_size') or self.size or self.y_size
        self.fig = plt.figure(figsize=(x_size, y_size))
        self.fig.patch.set_facecolor(self.colors.background)
        self.graph = self.fig.add_axes((self.left_margin, self.bottom_margin,
                                        (1 - self.left_margin - self.right_margin), self.graph_height))
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
            legend2 = self.graph.legend([ g for g in self.graph_lines[0].values() ],
                                        [ n for n in self.graph_lines[0].keys()],
                                        loc='center left' if self.legend else 'upper left')
            self.graph.add_artist(legend2)
            if self.legend:
                self.graph.legend(loc='upper left')
        self.first = True
        self.top_ax = self.graph
        self.build_extra()
        self.build_surtitle()

    def build_surtitle(self):
        row_labels = [ s[0] for s in self.surtitle ]
        col_values = [ [ s[1] ] for s in self.surtitle ]
        self.surtitles = self.top_ax.table(cellText=col_values, rowLabels=row_labels,
                                           fontsize=self.surtitle_font_size, edges='open',
                                           bbox=(self.surtitle_left_margin, self.surtitle_base,
                                                 self.surtitle_width,
                                                 self.surtitle_line_height * len(row_labels)))

    def build_extra(self):
        pass

    def pre_plot(self, x_size, y_size):
        pass

    def reset(self):
        for w in range(len(self.labels)):
            for l in self.lines:
                e = l.element
                for i in len(self.values[w][e]):
                    self.values[w][e][i] = np.nan

    def do_day(self, r):
        world_no, day = r
        if world_no != self.last_world:
            self.last_world = world_no
            self.last_day = 0
        while self.last_day < day:
            self.last_day += 1
            w = self.worlds[world_no]
            daily = w.get_daily(self.last_day)
            self.day_pre_extra(self.last_day, w, daily)
            if self.last_day > len(self.x_values) :
                self.extend_x()
            if self.first:
                self.first = False
                self.graph.set_ylim(initial_y_min, w.population)
            for l in self.lines:
                e = l.element
                self.values[world_no][e][self.last_day-1] = getattr(daily, e)
                self.graph_lines[world_no][e].set_ydata(self.values[world_no][e])
            self.day_extra(self.last_day, w, daily)

    def day_pre_extra(self, day, world, daily):
        pass

    def day_extra(self, day, world, daily):
        pass

    def extend_x(self):
        old_x = len(self.x_values)
        new_x = old_x + x_increase_delta
        self.x_values += [ d for d in range(old_x, new_x) ]
        self.graph.set_xlim(0, len(self.x_values))
        for w in range(len(self.values)):
            for l in self.lines:
                e = l.element
                self.values[w][e] += [ np.nan ] * x_increase_delta
                self.graph_lines[w][e].set_xdata(self.x_values)
                self.graph_lines[w][e].set_ydata(self.values[w][e])


