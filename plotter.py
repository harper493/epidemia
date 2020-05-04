import matplotlib.pyplot as plt
import os
import numpy as np
from utility import *

default_colors = 'blue,forestgreen,red,cyan,darkviolet,gold,' \
    'sienna,cornflowerblue,lime,lightcoral,turquoise,orange,sandybrown,' \
    'purple,yellowgreen,rosybrown,steelblue,crimson,black'

class plotter():

    def __init__(self):
        pass

    def plot(self, x, *data, title='', log=True, legend=True, file=None, show=False, format=None, props=None):
        size = props.get(int, 'plot', 'size') if props else None
        x_size = (props.get(int, 'plot', 'x_size') if props else None) or size
        y_size = (props.get(int, 'plot', 'y_size') if props else None) or size
        if x_size and y_size :
            plt.figure(figsize=(x_size, y_size))
        for d in data:
            style = d.get('style', '-')
            color = d.get('color', None)
            label = d.get('label', None)
            plt.plot(x, d['data'], label=label, linestyle=style, color=color)
        plt.xlabel('Days')
        plt.ylabel('People')
        if title :
            plt.title(title, fontsize=10)
        if log :
            plt.yscale('log')
        if legend:
            plt.legend(loc='upper left')
        if file :
            format = format or 'png'
            if '.' not in os.path.basename(file) :
                file = f'{file}.{format}'
            plt.savefig(file, bbox_inches='tight', format=format or 'png')
        plt.show()

    @staticmethod
    def make_colors(props):
        return (props.get('plot', 'colors') or default_colors).split(',')
