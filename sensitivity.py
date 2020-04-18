from utility import *

class sensitivity():

    class one_param() :

        def __init__(self, spec: str):
            self.pname, details = spec.split(':', 1)
            if ':' in details :
                if '*' in details:
                    self.mult = True
                    details = details.replace('*', ':')
                else:
                    self.mult = False
                self.values = None
                try :
                    r = details.split(':') + [None, None]
                    self.start, self.step, self.stop, stopper = r[:4]
                    if stopper is not None :
                        raise IndexError
                    self.start, self.step = number(self.start), number(self.step)
                    if self.stop is not None :
                        self.stop = number(self.stop)
                except IndexError:
                    raise ValueError(f"Incorrect parameter range '{spec}'")
            else :
                self.values = []
                for v in details.split(',') :
                    vv, m = (v.split('*') + [1])[:2]
                    self.values += [number(vv)] * number(m)
                self.start = 0
                self.index = 0
                self.step = 1
                self.stop = None
            if self.values :
                self.value = self.values[0]
            else :
                self.value = self.start

        def get(self):
            return (self.pname, self.value)

        def next(self) -> bool:
            """
            Advance to the next value.
            :return: True iff the value is within the stated range.
            """
            if self.values :
                self.index += 1
                try :
                    self.value = self.values[self.index]
                except IndexError :
                    self.value = self.values[-1]
                return self.index < len(self.values)
            if self.mult :
                self.value *= self.step
            else :
                self.value += self.step
            if self.stop is None :
                return True
            elif self.step>0 :
                return self.value <= (self.stop * 1.00001)
            else :
                return self.value >= (self.stop * 1.00001)

    def __init__(self, params, max_iterations: int=100):
        self.ranges = [ sensitivity.one_param(p) for p in params.split(';') ]

        self.max_iterations = max_iterations

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

                        Run through the listed values

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

    def __iter__(self):
        good = True
        i = 0
        while (good and i < self.max_iterations):
            yield [r.get() for r in self.ranges]
            good = [r.next() for r in self.ranges][0]
            i += 1

if __name__=='__main__' :
    s = sensitivity('foo:1:0.1:2')
    s = sensitivity('foo:1*2:16')
    s = sensitivity('foo:1,2,3,4')
    s = sensitivity('foo:1*10')
    s = sensitivity('foo:1,2*10,3')
