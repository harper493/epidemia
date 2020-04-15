#!/usr/bin/python3

from world import world
from properties import properties
from cluster import cluster
import random
from utility import *
from dynamic_table import dynamic_table

import cProfile
import re

def main() :
    props = properties('p1.props')
    cluster.make_cluster_info(props)
    w = world(props=props)
    show_cities(w)
    total_clusters = sum([ c.cluster_count for c in w.cities ])
    initial = props.get(int, 'initial')
    for p in random.choices(w.people, k=initial) :
        p.infect(0)
    print()
    t = dynamic_table((('Day', '%4d'), ('Infected', '%6d'), ('Rate', '%6.2f'),
                       ('Total', '%6d'), ('Rate', '%6.2f'),
                       ('Recovered', '%6d'), ('Immune', '%6d'),
                       ('Uninfected Cities', '%5d'),
                       ('Uninfected Clusters', '%6d'), ('%', '%6.2f'),
                       ('Susceptible Clusters', '%6d'), ('%', '%6.2f')))
    day = 0
    prev_infected = initial
    total_infected = initial
    prev_total = initial
    while w.infected >= prev_infected or w.infected > w.population//1000 :
        total_infected = w.infected + w.recovered - w.never_infected
        uninf_cities = sum([ 1 for c in w.cities if c.is_uninfected() ])
        uninf_clusters = sum([ c.get_uninfected_clusters() for c in w.cities ])
        susc_clusters = sum([ c.get_susceptible_clusters() for c in w.cities ])
        t.add(day, w.infected, w.infected/prev_infected, total_infected,
              total_infected / prev_total, w.recovered-w.never_infected, w.never_infected,
              uninf_cities, uninf_clusters, 100*uninf_clusters/total_clusters,
              susc_clusters, 100*susc_clusters/total_clusters)
        prev_infected = w.infected
        prev_total = total_infected
        day += 1
        w.one_day(day)

def show_cities(w) :
    print()
    t = dynamic_table((('City', '%5d'), ('Location', '%20s'), ('Population', '%6d'), ('Size', '%6.2f')))
    for c in w.cities :
        t.add(c.name, str(c.location), c.pop, c.size)

main()
#cProfile.run('main()')
