from enum import Enum
import random

class person_state(Enum) :
    S = 0    # susceptible
    G = 1    # gestating
    I = 2    # infected
    R = 3    # recovered
    D = 4    # dead
    X = 5    # auto-immune, exposed but never infected

class person(object) :

    def __init__(self, name, world_):
        self.name = name
        self.world_ = world_
        self.city = world_.get_random_city()
        self.location = self.city.get_random_location()
        self.clusters = self.city.pick_clusters(self.location)
        self.state = person_state.S
        self.infected = None

    def __str__(self):
        return 'person %s state %s city %s location %s' % \
               (self.name, str(self.state)[-1], self.city.name, self.location)

    def is_susceptible(self) -> bool:
        return self.state==person_state.S

    def is_infected(self) -> bool:
        return self.state==person_state.I

    def is_recovered(self) -> bool:
        return self.state==person_state.R

    def infectious(self, day):
        if day - self.infected >= self.world_.recovery_dist() :
            self.recover()
        else :
            for cl in self.clusters.values() :
                cl.expose()
            self.city.expose()

    def expose(self, day: int):
        if self.is_susceptible() :
            r = random.random()
            risk = self.city.get_exposure()
            for cl in self.clusters.values() :
                risk += cl.get_exposure()
            auto_immunity = self.world_.get_auto_immunity()
            if r < risk :
                if r < risk *self.world_.get_auto_immunity() :
                    self.recover()
                else :
                    self.infect(day)

    def infect(self, day):
        self.infected = day
        self.city.infect_one(self)
        for cl in self.clusters.values() :
            cl.infect_one(self)
        self.state = person_state.I

    def recover(self):
        self.city.recover_one(self)
        for cl in self.clusters.values() :
            cl.recover_one(self)
        self.state = person_state.R
        