import matplotlib.pyplot as plt
from matplotlib import animation
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
from dataclasses import dataclass
from geometry import *
import numpy as np
import os
import time
from utility import *
from constructor import constructor

@dataclass
class line_info():
    element: str
    color: str = None
    style: str = None

class dynamic_plot():

    @dataclass
    class colors():
        background = 'ivory'
        button = 'linen'
        button_hover = 'bisque'
        susceptible = 'deepskyblue'
        infected = 'red'
        recovered = 'lime'
        graph_infected = 'darkorange'
        graph_total = 'red'
        graph = 'blue,forestgreen,red,cyan,darkviolet,gold,' \
                'sienna,cornflowerblue,lime,lightcoral,turquoise,orange,magenta,' \
                'purple,yellowgreen,rosybrown,steelblue,crimson,black'

        def load(self, props):
            for pname, pvalue in props:
                if pname[0]=='color':
                    setattr(self, pname[-1], pvalue)
            self.graph = self.graph.split(',')

    arg_table = {
        'title' : (str, ''),
        'log_scale' : (bool, True),
        'legend': (str, True),
        'file': (str, ''),
        'show': (bool, False),
        'format': (str, ''),
        'props': None,
        'total_color': None,
        'infected_color': None,
        'lines': None,
    }

    def __init__(self, **kwargs):
        constructor(dynamic_plot.arg_table, args=kwargs).apply(self)
        if self.lines is None:
            self.lines = (line_info('total', style='solid'),
                          line_info('infected', style='dashed'))
        self.colors = dynamic_plot.colors()
        self.colors.load(self.props)
        
    def pre_plot(self, x_size=None, y_size=None, size=None):
        size = self.props.get(int, 'plot', 'size') or size
        x_size = self.props.get(int, 'plot', 'x_size') or size or x_size
        y_size = self.props.get(int, 'plot', 'y_size') or size or y_size
        self.fig = plt.figure(figsize=(x_size, y_size))
        self.fig.patch.set_facecolor(self.colors.background)



