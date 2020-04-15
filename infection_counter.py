class infection_counter() :

    def __init__(self, parent=None):
        self.parent = parent
        self.infected = 0
        self.recovered = 0
        self.never_infected = 0

    def infect_one(self, p: 'person'):
        self.infected += 1
        if self.parent :
            self.parent.infect_one(p)

    def recover_one(self, p: 'person') :
        self.recovered += 1
        if p.is_infected() :
            self.infected -= 1
        else :
            self.never_infected += 1
        if self.parent :
            self.parent.recover_one(p)

    def is_uninfected(self):
        return self.infected+self.recovered==0


    def is_susceptible(self):
        return self.infected+self.recovered < self.pop

