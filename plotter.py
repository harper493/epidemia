import matplotlib.pyplot as plt
import numpy as np
from utility import *


class plotter():

    def __init__(self):
        pass

    def plot(self, x, *data, title='', log=True):

        for d in data:
            plt.plot(x, d[1], label=d[0])
        plt.xlabel('Days')
        plt.ylabel('People')
        if title :
            plt.title(title, fontsize=10)
        if log :
            plt.yscale('log')
        plt.legend(loc='upper left')
        plt.show()
