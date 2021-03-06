from enum import Enum
from utility import *
import random

class person_state(Enum) :
    S = 0    # susceptible
    G = 1    # gestating
    I = 2    # infected
    R = 3    # recovered
    D = 4    # dead
    X = 5    # auto-immune, exposed but never infected

class person(object) :

    def __init__(self, name, world_, city=None):
        self.name = name
        self.world_ = world_
        self.city = city if city is not None else world_.get_random_city()
        self.location = self.city.get_random_location()
        self.clusters = self.city.pick_clusters(self.location)
        self.state = person_state.S
        self.infected = None
        for cl in self.clusters.values():
            cl.enroll(self)

    def __str__(self):
        return 'person %s state %s city %s location %s' % \
               (self.name, str(self.state)[-1], self.city.name, self.location)

    def is_susceptible(self) -> bool:
        return self.state==person_state.S

    def is_gestating(self) -> bool:
        return self.state==person_state.G

    def is_infected(self) -> bool:
        return self.state==person_state.I

    def is_recovered(self) -> bool:
        return self.state==person_state.R

    def is_immune(self) -> bool:
        return self.state==person.state.X

    def infectious(self, day: int):
        assert (self.is_infected())
        if day - self.infected >= self.world_.recovery_dist.get() :
            self.recover()
            assert(self.is_recovered())
        else :
            for cl in self.clusters.values() :
                cl.expose()
            if random.random() < self.world_.travel :
                self.city.get_destination().expose()
            else :
                self.city.expose()
            assert (self.is_infected())

    def expose(self, day: int):
        if self.is_susceptible() :
            if random.random() < self.world_.travel :
                risk = self.city.get_destination().get_exposure()
            else :
                risk = self.city.get_exposure()
            r = random.random()
            for cl in self.clusters.values() :
                risk += cl.get_exposure()
            auto_immunity = self.world_.get_auto_immunity()
            if r < risk :
                if r < risk *self.world_.get_auto_immunity() :
                    self.immunise()
                else :
                    self.gestate(day)

    def gestating(self, day: int) -> bool:
        if day - self.infected >= self.world_.gestating_dist.get() :
            self.infect(day)
            return True
        else :
            return False

    def gestate(self, day: int):
        assert(self.is_susceptible())
        self.infected = day
        self.city.gestate_one(self)
        for cl in self.clusters.values() :
            cl.gestate_one(self)
        self.state = person_state.G


    def infect(self, day):
        if self.infected is None :
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

    def immunise(self):
        assert_(self.is_susceptible())
        self.city.immunise_one(self)
        for cl in self.clusters.values() :
            cl.immunise_one(self)
        self.state = person_state.X

        