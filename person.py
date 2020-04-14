from enum import Enum

class person_state(Enum) :
    S = 0
    I = 1
    R = 2

class person(object) :

    def __init__(self, name, world_):
        self.name = name
        self.city = world_.get_random_city()
        self.location = self.city.get_random_location()
        self.clusters = self.city.pick_clusters(self.location)
        self.state = person_state.S

    def __str__(self):
        return 'person %s state %s city %s location %s' % \
               (self.name, str(self.state)[-1], self.city.name, self.location)

