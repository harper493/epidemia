#!/usr/bin/python3.8

from world import world
from properties import properties
from cluster import cluster
from sensitivity import sensitivity
from utility import *
from dynamic_table import dynamic_table
from     plotter import plotter
from argparser import argparser
from math import *

import cProfile

PROFILE = True

def main() :
    args = argparser()
    propfiles = ['base.props']
    if args.props_file :
        propfiles.append(args.props_file)
    props = properties(*propfiles, cmd_args=args.extra_props)
    if args.sensitivity :
        run_sensitivity(args, props)
    else :
        run_one(args, props)

def run_one(args, props) :
    cluster.make_cluster_info(props)
    w = world(props=props)
    if args.very_verbose :
        show_cities(w)
    total_clusters = sum([c.get_leaf_clusters() for c in w.cities])
    print()
    if args.verbose :
        t = dynamic_table((('Day', '%4d'), ('Infected', '%6d'), ('Rate', '%6.2f'),
                           ('Total', '%6d'), ('%', '%6.2f'), ('Rate', '%6.2f'), ('Delta', '%5d'),
                           ('Recovered', '%6d'), ('Delta', '%5d'), ('Immune', '%6d'),
                           ('Untouched Cities', '%5d'),
                           ('Untouched Clusters', '%6d'), ('%', '%6.2f'),
                           ('Susceptible Clusters', '%6d'), ('%', '%6.2f')))
    while w.one_day() :
        if args.verbose :
            untouched_cities = sum([ 1 for c in w.cities if c.is_untouched() ])
            untouched_clusters = sum([ c.get_untouched_clusters() for c in w.cities ])
            susc_clusters = sum([ c.get_susceptible_clusters() for c in w.cities ])
            t.add(w.day, w.infected, w.growth,
                  w.total_infected, 100 * w.total_infected / w.population, w.total_infected / (w.prev_total or 1),
                  w.total_infected - w.prev_total,
                  w.recovered, w.recovered - w.prev_recovered, w.immune,
                  untouched_cities, untouched_clusters, 100 * untouched_clusters / total_clusters,
                  susc_clusters, 100 * susc_clusters / total_clusters)
            if w.total_infected > 0.95 * w. population :
                a=1
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
    t = dynamic_table((('City', '%6s'), ('Location', '%20s'), ('Population', '%6d'), ('Size', '%6.2f')))
    for c in w.cities :
        t.add(c.name, str(c.location), c.pop, c.size)

def plot_results(w) :
    from_, to = w.get_interesting()
    p = plotter()
    title = f'Population {w.population} Infectiousness {w.get_infectiousness()} Auto-Immunity {w.get_auto_immunity()}'
    title += f'\nMax Days to Double {w.days_to_double:.1f}'
    x = w.get_days()[from_:to]
    data = [ (var_to_title(v), w.get_data(v)[from_:to]) for v in ('total_infected', 'infected') ]
    p.plot(x, *data, title=title)

def run_sensitivity(args, props):
    sens = sensitivity(args.sensitivity)
    cols = [(var_to_title(n), '%6.3f') for n in sens.get_variables()]
    cols += [('Max Infected', '%6d'), ('%', "%5.2f"), ('Total Infected', '%6d'), ('%', "%5.2f"),
             ('Days to Double', '%5.1f'), ('Days to Peak', '%3d')]
    t = dynamic_table(cols)
    params = []
    results = []
    for ss in sens :
        params.append(ss[0])
        props.add_properties('\n'.join([f'{v[0]}={v[1]}' for v in ss]))
        cluster.make_cluster_info(props)
        w = world(props=props)
        w.run()
        results.append(w)
        values = [ sss[1] for sss in ss ]
        values += [ w.max_infected, 100*w.max_infected/w.population,
              w.total_infected, 100*w.total_infected/w.population,
              w.days_to_double, w.highest_day ]
        t.add(*values)
    if args.plot :
        from_ = max([ w.get_interesting()[0] for w in results ])
        to = max([ w.get_interesting()[1] for w in results ])
        plot = plotter()
        x = range(from_, to)
        data = []
        for p, w in zip(params, results) :
            d = w.get_data('total_infected')[from_:to]
            while len(d) < len(x) :
                d.append(d[-1])
            data.append((f'{p[1]:.3f}', d))
        titles = []
        for t in ('population', 'infectiousness', 'auto_immunity') :
            if t not in sens.get_variables() :
                titles.append(f'{var_to_title(t)} = {getattr(results[0], t)}')
        title = ' '.join(titles) + f'\nVarying {var_to_title(p[0])}'
        plot.plot(x, *data, title=title)


#cProfile.run('main()')
main()
