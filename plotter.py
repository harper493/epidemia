import matplotlib.pyplot as plt
import numpy as np

class plotter() :

    def __init__(self, world_, values) :
        self.world_, self.values = world_, values
        self.days = self.world_.get_days()

    def plot(self, from_=0, to=0):
        data = self.world_.get_data(self.values[0])
        to = to or len(self.days)
        plt.plot(self.days[from_:to], data[from_:to])
        plt.xlabel('Days')
        plt.ylabel(self.values[0])
        plt.yscale('log')
        plt.show()