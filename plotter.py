import matplotlib.pyplot as plt
import numpy as np
from utility import *


class plotter():

    def __init__(self, world_, *values):
        self.world_, self.values = world_, values
        self.days = self.world_.get_days()

    def plot(self, from_=0, to=0, title='', log=True):
        to = to or len(self.days)
        ax = plt.subplot('111')
        for v in self.values:
            if isinstance(v, str) :
                label = v
                data = self.world_.get_data(v)
            else :
                label = v[0]
                data = self.world_.get_data(v[1])
            ax.plot(self.days[from_:to], data[from_:to], label=var_to_title(label))
        plt.xlabel('Days')
        plt.ylabel('People')
        if title :
            plt.title(title, fontsize=10)
        if log :
            plt.yscale('log')
        ax.legend(loc='upper left')
        plt.show()
