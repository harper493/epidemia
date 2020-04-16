"""
reciprocal module: defines reciprocal class which generates a reciprocal-like random distribution, given:

-- number of values to produce
-- minimum value
-- maximum value
-- sum of values

As stated this is over constrained. It first generates a random sequence, then adjusts the power
in the expression 1/n^p so hat it can manipulate the values to get everything to work out.
"""

import random
import statistics
import math
import sys
import matplotlib.pyplot as plt

DEFAULT_TOLERANCE = 0.1     # percent
MAX_POWER = 10000000

class reciprocal(object) :

    def __init__(self, count, min_, max_, sum_):
        self.count, self.min_, self.max_, self.sum_ = count, min_, max_, sum_
        if ((self.sum_ - self.max_ - self.min_) // (self.count-2) < self.min_ +1) \
            or (self.max_ * self.count < self.sum_) :
            raise ValueError('incompatible arguments')
        self.min_ = min(self.min_, self.sum_ / (self.count*1.1))
        self.tolerance = 1 + DEFAULT_TOLERANCE/100

    def get(self) :
        """
        get() - get a sequence matching the parameters specified when the object was created
        """
        r = self.max_/self.min_
        good = False
        while not good:
            s = [ random.uniform(1, r) for i in range(self.count) ]
            m = 1/min(s)
            s = [ n*m for n in s ]
            p = 1.0
            pp = 0.1
            prev_sum, prev_prev_sum = None, 1
            while p<MAX_POWER and p>1/MAX_POWER :
                ss = [ pow(n, p) for n in s ]
                m = (r-1)/ (max(ss)-1)
                ss = [ ((n-1) * m)+1 for n in ss ]
                t = sum(self._make_result(ss))
                if prev_sum is None :
                    prev_sum = t
                if t > self.sum_ * self.tolerance :
                    if prev_sum < self.sum_ or p<=pp*2:
                        pp /= 10
                    p -= pp
                elif t < self.sum_ / self.tolerance :
                    if prev_sum > self.sum_ :
                        pp /= 10
                    p += pp
                else :
                    good = True
                    break
                prev_prev_sum = prev_sum
                prev_sum = t
                #print('****', p, self.min_, t, ss)
        r = self._make_result(ss)
        r = self._adjust_total(r)
        self.power = p
        return r

    def _make_result(self, s):
        return [ int(self.max_ / n) for n in s ]

    def _adjust_total(self, s):
        """
        adjust_total - the random sequence is adjusted until it is within a close tolerance
        of the requested total. This function tweaks all the values to get it exactly right.
        """
        s.sort(reverse=True)
        diff = self.sum_ - sum(s)
        delta = (int((abs(diff) + len(s) - 3) / (len(s) - 2))) * (1 if diff>0 else -1)
        for i in range(1, len(s)-1) :
            s[i] += delta
            diff -= delta
            if abs(delta) > abs(diff) :
                delta = diff
        return s

if __name__=="__main__" :
    mean = 9
    sd = 2
    mu = math.log(mean**2/math.sqrt(mean**2 + sd **2))
    sigma = math.sqrt(math.log(1+(sd/mean)**2))
    results = []
    for i in range(10000) :
        r = random.lognormvariate(mu, sigma)
        results.append(r)
    amean = statistics.mean(results)
    asd = statistics.stdev(results)
    print(f'{mean=} {sd=} {mu=} {sigma=} {amean=} {asd=}')
    num_bins = 40
    n, bins, patches = plt.hist(results, num_bins, facecolor='blue', alpha=0.5)
    plt.show()
    sys.exit(0)
    for i in range(10) :
        r = reciprocal(200, 1000, 500000,1000000)
        rr = r.get()
        print ("sum %d mean %d stddev %.0f harm %.0f median %.0f power %.3f [%s]" %
               (sum(rr), statistics.mean(rr), statistics.pstdev(rr),
                statistics.harmonic_mean(rr), statistics.median(rr), r.power,
               str(rr)))


