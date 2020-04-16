#!/usr/bin/python3

from world import world
from properties import properties
from cluster import cluster
import random
from utility import *
from dynamic_table import dynamic_table
from plotter import plotter
from argparser import argparser

import cProfile
import re

PROFILE = True

def main() :
    args = argparser()
    propfiles = ['base.props']
    if args.props_file :
        propfiles.append(args.props_file)
    props = properties(*propfiles)
    cluster.make_cluster_info(props)
    w = world(props=props)
    if args.very_verbose :
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
        if args.verbose :
            uninf_cities = sum([ 1 for c in w.cities if c.is_uninfected() ])
            uninf_clusters = sum([ c.get_uninfected_clusters() for c in w.cities ])
            susc_clusters = sum([ c.get_susceptible_clusters() for c in w.cities ])
            t.add(w.day, w.infected, w.growth, w.total_infected, 100 * w.total_infected/w.population,
                  w.total_infected / w.prev_total, w.immune, w.never_infected,
                  uninf_cities, uninf_clusters, 100*uninf_clusters/total_clusters,
                  susc_clusters, 100*susc_clusters/total_clusters)
    print('\nMax Infected: %d (%.2f%%) Total Infected: %d (%.2f%%) Max Growth: %.2f%% Days to Double: %.1f Days to Peak: %d' \
          % (w.max_infected, 100*w.max_infected/w.population,
             w.prev_total, 100*w.prev_total/w.population,
             (w.max_growth-1)*100, w.days_to_double, w.highest_day))
    print('\nPopulation: %d Setup Time: %.2fS Days: %d in %.2fS %.3f S/day' \
        % (w.population, w.setup_time, w.day, w.run_time,
           w.run_time/w.day))
    if args.plot :
        plot_results(w)

def show_cities(w) :
    print('\n')
    t = dynamic_table((('City', '%5d'), ('Location', '%20s'), ('Population', '%6d'), ('Size', '%6.2f')))
    for c in w.cities :
        t.add(c.name, str(c.location), c.pop, c.size)

def plot_results(w) :
    from_ = None
    to = len(w.get_days())
    for d in w.get_days():
        ti = w.get_data_point('total_infected', d)
        if from_:
            if w.get_data_point('infected', d) < ti // 10:
                to = d
                break
        elif ti > w.population // 50:
            from_ = d

    p = plotter(w, 'total_infected', 'infected')
    p.plot(from_=from_, to=to)


cProfile.run('main()')
#main()
