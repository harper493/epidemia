from utility import *
from math import fsum
import re

class sensitivity():

    class one_param() :

        def __init__(self, spec: str):
            try :
                m = re.match(r'^([+;])([^:]+):(.*)', spec)
                if not m :
                    raise ValueError
                prod, self.pname, self.text = m.group(1, 2, 3)
                self.product = (prod=='+')
                details = self.text
                if ':' in details :
                    if '*' in details:
                        self.mult = True
                        details = details.replace('*', ':')
                    else:
                        self.mult = False
                    self.values = None
                    r = details.split(':') + [None, None]
                    self.start, self.step, self.stop, stopper = r[:4]
                    if stopper is not None :
                        raise IndexError
                    self.start, self.step = number(self.start), number(self.step)
                    if self.stop is not None :
                        self.stop = number(self.stop)
                else :
                    self.values = []
                    for v in details.split(',') :
                        vv, m = (v.split('*') + [1])[:2]
                        self.values += [number(vv)] * number(m)
                    self.start = 0
                    self.step = 1
                    self.stop = None
                self.reset()
            except (IndexError, ValueError):
                raise ValueError(f"Incorrect parameter range '{spec}'")

        def __str__(self):
            return self.text

        def get(self):
            return (self.pname, self.value)

        def reset(self):
            self.index = 0
            if self.values:
                self.value = self.values[0]
            else:
                self.value = self.start

        def next(self) -> bool:
            """
            Advance to the next value.
            :return: True iff the value is within the stated range.
            """
            self.index += 1
            if self.values :
                try :
                    self.value = self.values[self.index]
                except IndexError :
                    self.value = self.values[-1]
                return self.index < len(self.values)
            if self.mult :
                self.value *= self.step
            else :
                self.value = self.start + self.step * self.index#fsum([self.start] + [self.step] * self.index)
            if self.stop is None :
                return True
            elif self.step>0 :
                return self.value <= (self.stop * 1.00001)
            else :
                return self.value >= (self.stop * 1.00001)

        def will_stop(self):
            return self.values or self.stop is not None

    def __init__(self, params, max_iterations: int=100):
        r = ['+'] + re.split(r'([+;])', params)
        if '' in r :
            raise ValueError(f"Syntax error in sensitivity description: '{params}'")
        r2 = [ m+t for m,t in zip(r[::2], r[1::2])]
        self.ranges = [ sensitivity.one_param(p) for p in r2 ]
        self.named_ranges = { r.pname : r for r in self.ranges }
        self.max_iterations = max_iterations
        if not self.ranges[0].will_stop() :
            raise ValueError(f"First range must have a termination condition '{str(self.ranges[0])}'")

    def run(self, params: str):
         """
         Call the given function repeatedly with a single parameter which is a list of
         tuples of the form (parameter name, parameter value)
         Run sensitivity analysis over given parameter range(s)
         :param string describing the parameters to be varied. One or more param specs separated by ';'.
                Each of them is of one of the forms:
                    param_name:start:step:end

                        Run the parameter from 'start' by 'increments of 'step' up to (including) 'stop'

                    param_name*start:mult:end

                        As above but multiply each time instead of adding

                    param_name:1,2,3,4,...

                        Run through the listed values. A value can also be of the form '2*3' which
                        is equivalent to repeating the value 2, 3 times. If a list is too short,
                        the last number is repeated as necessary.

                    param_name:start:step
                    param_name*start:step

                        As above. Can only be used for the second or subsequent parameter.

                    If multiple parameters are specified, they will be increased in step. If the ranges
                    are not the same length, the first one wins.

                Examples:

                infectiousness:0.005:0.001:0.01
                city_count:10*2:80
                auto_immunity:0.2,0.5,0.6,0.7
                infectiousness:0.005:0.001:0.01;auto_immunity:0.5:0.05

                For more examples, see the unit tests at the end of this file.

         :return: a list of the results from each individual run
         """
         ranges = [ sensitivity.one_param(p) for p in params.split(';') ]
         good = True
         i = 0
         while (good and i<self.max_iterations) :
             (self.fn)([ r.get() for r in ranges ])
             good = [ r.step() for r in ranges ][0]
             i += 1

    def get_variables(self):
        return [ r.pname for r in self.ranges ]

    def get_one(self, p):
        return self.named_ranges[p].value

    def __iter__(self):
        good = True
        count = 0
        while (count < self.max_iterations):
            yield [r.get() for r in self.ranges]
            top = len(self.ranges)
            for ii, r in enumerate(reversed(self.ranges)) :
                i = len(self.ranges) - 1 - ii
                if r.product :
                    g = [ r2.next() for r2 in self.ranges[i:top] ][0]
                    if g :
                        break
                    for r in self.ranges[i:top] :
                        r.reset()
                    top = i
            else :
                break
            count += 1

def _test_one(s) :
    sens = sensitivity(s)
    print(s, end=': ')
    for ss in sens :
        print(ss, end=' ')
    print('\n')

if __name__=='__main__' :
    _test_one('foo:1,2+bah:10,11+bim:100,101;boo:1000')
    _test_one('foo:1:0.1:2')
    _test_one('foo:1*2:16')
    _test_one('foo:1,2,3,4')
    _test_one('foo:1*10')
    _test_one('foo:1,2*10,3')
    _test_one('foo:1:1:10;bah:2:2')
    _test_one('foo:1,2,3,4,5;bah:2,4')
    _test_one('foo:1:1:3+bah:10:1:12')
