from math import *
import random

class lognormal() :

    def __init__(self, mean, sd):
        self.mean, self.sd = mean, sd
        self.mu = log(mean ** 2 / sqrt(mean ** 2 + sd ** 2))
        self.sigma = sqrt(log(1 + (sd / mean) ** 2))

    def __call__(self):
        return random.lognormvariate(self.mu, self.sigma)