from world import world
from properties import properties
from cluster import cluster
import random
from utility import *
from dynamic_table import dynamic_table

def main() :
    props = properties('p1.props')
    cluster.make_cluster_info(props)
    w = world(props=props)
    initial = props.get(int, 'initial')
    for p in random.choices(w.people, k=initial) :
        p.infect(0)
    print()
    t = dynamic_table((('Day', '%4d'), ('Infected', '%6d'), ('Rate', '%6.2f'), ('Recovered', '%6d'), ('Immune', '%6d')))
    day = 0
    prev_infected = initial
    while w.infected :
        t.add(day, w.infected, w.infected/prev_infected, w.recovered, w.never_infected)
        prev_infected = w.infected
        day += 1
        w.one_day(day)

main()