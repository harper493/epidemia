from utility import *

class infection_counter() :

    def __init__(self, parent=None):
        self.parent = parent
        self.gestating = 0
        self.infected = 0
        self.recovered = 0
        self.immune = 0
        self.susceptible = None

    def infect_one(self, p: 'person'):
        if p.is_susceptible() :
            if self.susceptible is None:
                self.susceptible = self.pop
            self.susceptible -= 1
        else :
            assert(p.is_gestating())
            self.gestating -= 1
        self.infected += 1
        if self.parent :
            self.parent.infect_one(p)

    def gestate_one(self, p: 'person'):
        assert(p.is_susceptible())
        if self.susceptible is None :
            self.susceptible = self.pop
        self.susceptible -= 1
        self.gestating += 1
        if self.parent :
            self.parent.gestate_one(p)

    def recover_one(self, p: 'person') :
        assert_(p.is_infected())
        self.infected -= 1
        self.recovered += 1
        if self.parent :
            self.parent.recover_one(p)

    def immunise_one(self, p: 'person'):
        assert(p.is_susceptible())
        if self.susceptible is None :
            self.susceptible = self.pop
        self.susceptible -= 1
        self.immune += 1
        if self.parent :
            self.parent.immunise_one(p)

    def is_untouched(self):
        return self.susceptible==self.pop

    def is_susceptible(self):
        try :
            return self.susceptible > 0
        except TypeError :
            self.susceptible = self.pop
            return self.susceptible > 0

    def _check(self):
        if self.susceptible is None :
            self.susceptible = self.pop
        assert_(self.susceptible + self.infected + self.immune + self.recovered == self.pop)
        assert_(getattr(self, 'people', None) is None or self.pop==len(self.people))
        assert_(self.infected==len(self.infectees))

