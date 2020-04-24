import matplotlib.pyplot as plt
import os
import numpy as np
from utility import *


class plotter():

    def __init__(self):
        pass

    def plot(self, x, *data, title='', log=True, legend=True, file=None, show=False, format=None):
        for d in data:
            a = len(d)
            b = d[0]
            plt.plot(x, d[1], label=d[0])
        plt.xlabel('Days')
        plt.ylabel('People')
        if title :
            plt.title(title, fontsize=10)
        if log :
            plt.yscale('log')
        if legend:
            plt.legend(loc='upper right')
        if file :
            format = format or 'png'
            if '.' not in os.path.basename(file) :
                file = f'{file}.{format}'
            plt.savefig(file, bbox_inches='tight', format=format or 'png')
        plt.show()
