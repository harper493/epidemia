#!/usr/bin/python3

from world import world
from properties import properties
from cluster import cluster
import random
from utility import *
from dynamic_table import dynamic_table

import cProfile
import re

PROFILE = False

def main() :
    props = properties('p1.props')
    cluster.make_cluster_info(props)
    w = world(props=props)
    show_cities(w)
    total_clusters = sum([ c.cluster_count for c in w.cities ])
    print()
    t = dynamic_table((('Day', '%4d'), ('Infected', '%6d'), ('Rate', '%6.2f'),
                       ('Total', '%6d'), ('%', '%6.2f'), ('Rate', '%6.2f'),
                       ('Recovered', '%6d'), ('Immune', '%6d'),
                       ('Uninfected Cities', '%5d'),
                       ('Uninfected Clusters', '%6d'), ('%', '%6.2f'),
                       ('Susceptible Clusters', '%6d'), ('%', '%6.2f')))
    while w.one_day() :
        uninf_cities = sum([ 1 for c in w.cities if c.is_uninfected() ])
        uninf_clusters = sum([ c.get_uninfected_clusters() for c in w.cities ])
        susc_clusters = sum([ c.get_susceptible_clusters() for c in w.cities ])
        t.add(w.day, w.infected, w.growth, w.total_infected, 100 * w.total_infected/w.population,
              w.total_infected / w.prev_total, w.immune, w.never_infected,
              uninf_cities, uninf_clusters, 100*uninf_clusters/total_clusters,
              susc_clusters, 100*susc_clusters/total_clusters)
    print('\nMax Infected: %d Total Infected: %d Max Growth: %.2f%% Days to Double: %.1f Days to Peak: %d' \
          % (w.max_infected, w.prev_total, (w.max_growth-1)*100, w.days_to_double, w.highest_day))

def show_cities(w) :
    print('\n')
    t = dynamic_table((('City', '%5d'), ('Location', '%20s'), ('Population', '%6d'), ('Size', '%6.2f')))
    for c in w.cities :
        t.add(c.name, str(c.location), c.pop, c.size)

if PROFILE :
    cProfile.run('main()')
else :
    main()
