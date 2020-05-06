import random
import math
import re

DEFAULT_PLACES = 3

class point(object) :

    def __init__(self, x=0, y=0, string=None):
        if string:
            self.parse(string)
        else:
            self.x, self.y = float(x), float(y)

    def __str__(self, places=DEFAULT_PLACES):
        fmt = '%%.%df' % (places)
        return '(%s, %s)' % (fmt % self.x, fmt % self.y)

    def __add__(self, other):
        return point(self.x+other.x, self.y+other.y)

    def __sub__(self, other):
        return point(self.x-other.x, self.y-other.y)

    def __mult__(self, scalar):
        return point(self.x*scalar, self.y*scalar)

    def __div__(self, scalar):
        return point(self.x/scalar, self.y/scalar)

    def dist(self, other):
        delta = self - other
        return math.sqrt(delta.x*delta.x + delta.y*delta.y)

    def apply(self, fn):
        return point(fn(self.x), fn(self.y))

    def parse(self, value):
        m = re.match(r'(?:\()?([0-9.]+),([0-9.]+)(?:\))?', value)
        self.x = self.y = 0
        if m:
            try:
                self.x = float(m.group(1))
                self.y = float(m.group(2))
            except ValueError: pass

class geometry(object) :

    def __init__(self, size_x, size_y) :
        self.size_x, self.size_y = size_x, size_y

    def random_location(self) :
        return point(random.uniform(0, self.size_x), random.uniform(0, self.size_y))

