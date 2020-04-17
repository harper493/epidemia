from math import *
import random
import statistics
import matplotlib.pyplot as plt

class lognormal() :

    def __init__(self, mean, sd, min_=None):
        self.offset = mean/2 if min_ is None else min_
        self.mean = mean - self.offset
        self.sd = sd
        self.mu = log(self.mean ** 2 / sqrt(self.mean ** 2 + self.sd ** 2))
        self.sigma = sqrt(log(1 + (self.sd / self.mean) ** 2))

    def get(self):
        result = random.lognormvariate(self.mu, self.sigma) + self.offset
        return result

if __name__=="__main__" :
    ln = lognormal(9,2)
    results = []
    for i in range(10000) :
        r = ln.get()
        results.append(r)
    amean = statistics.mean(results)
    asd = statistics.stdev(results)
    num_bins = 40
    n, bins, patches = plt.hist(results, num_bins, facecolor='blue', alpha=0.5)
    plt.show()
