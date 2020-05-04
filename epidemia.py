#!/usr/bin/python3.8

from world import world
from properties import properties
from cluster import cluster
from sensitivity import sensitivity
from utility import *
from dynamic_table import dynamic_table
from plotter import plotter
from argparser import argparser
from math import *
from datetime import datetime
from fast_world import fast_world
import functools
import itertools

import cProfile

def _f(*args, **kwargs) :
    return dynamic_table.field(*args, **kwargs)

log_fields = (
    _f('day', '%4d'),
    _f('infected', '%6d'),
    _f('growth', '%6.2f'),
    _f('total_infected', '%6d'),
    _f('%', '%6.2f', lambda w: 100 * w.total_infected / w.population),
    _f('Growth', '%6.2f', lambda w: w.total_infected / (w.prev_total or 1)),
    _f('Delta', '%5d', lambda w: w.total_infected - w.prev_total),
    _f('recovered', '%6d'),
    _f('Delta', '%5d', lambda w: w.recovered - w.prev_recovered),
    _f('immune', '%6d'),
    _f('untouched_cities', '%5d'),
    _f('untouched_clusters', '%6d'),
    _f('%', '%6.2f', lambda w: 100 * w.untouched_clusters / (w.total_clusters or 1)),
    _f('susceptible_clusters', '%6d'),
    _f('%', '%6.2f', lambda w: 100 * w.susceptible_clusters / (w.total_clusters or 1)),
)

city_fields = (
    _f('City', '%6s', lambda c: c.name),
    _f('location', '%20s'),
    _f('pop', '%6d'),
    _f('size', '%6.2f'),
    _f('infected', '%6d')
)

summary_fields = (
    _f('max_infected', '%6d'),
    _f('%', "%5.2f", lambda w: 100 * w.max_infected / w.population),
    _f('total_infected', '%6d'),
    _f('%', "%5.2f", lambda w: 100 * w.total_infected / w.population),
    _f('days_to_double', '%5.1f'),
    _f('Days to Peak', '%3d', lambda w: w.highest_day),
)

class epidemia() :
    
    def __init__(self):
        self.args = argparser()
        propfiles = ['base.props']
        if self.args.props_file :
            propfiles.append(self.args.props_file)
        self.props = properties(*propfiles, cmd_args=self.args.extra_props)
        cluster.make_cluster_info(self.props)
        if self.args.log_path or self.args.output:
            self.log_path = self.args.log_path or '.'
            if self.log_path[-1] != '/' :
                self.log_path += '/'
            self.log_filename = self.args.output or \
                                'epidemia_{}'.format(datetime.now().strftime('%Y%m%dT%H%M%S_%f')[:-3])
            self.log_file = open(f'{self.log_path}{self.log_filename}.log', 'w')
        else :
            self.log_path = None
            self.log_filename = None
            self.log_file = None

    def write_log_header(self, file):
        if file :
            file.write(self.args.command_line)
            file.write(f"\n\nProperties from file '{self.args.props_file}':\n\n")
            file.write(self.props.dump())
            file.write('\n\n')

    def run(self):
        self.write_log_header(self.log_file)
        if self.args.sensitivity or self.args.repeat:
            self.run_sensitivity()
        else :
            self.run_one()
        if self.log_file :
            self.log_file.close()

    def run_one(self) :
        w = fast_world(props=self.props)
        if True:
            w.run()
        else:
            if self.args.very_verbose :
                self.show_cities(w)
            print()
            t = dynamic_table(log_fields, file=self.log_file, console=self.args.console)
            w.run(logger=lambda w: t.add_line(w))
            t.write((f'\nMax Infected: {w.max_infected:d}',
                    f'({100*w.max_infected/w.population:.2f}%)',
                    f'Total Infected: {w.total_infected:d}',
                    f'({100*w.total_infected/w.population:.2f}%)',
                    f'Max Growth: {100*(w.max_growth-1):.2f}%',
                    f'Days to Double: {w.days_to_double:.1f}',
                    f'Days to Peak: {w.highest_day}'))
            t.write((f'Population: {w.population:d}',
                     f'Setup Time: {w.setup_time:.2f}S',
                     f'Days: {w.day:d} in {w.run_time:.2f}S { w.run_time/w.day:.3f} S/day'))
        if self.args.plot :
            self.plot_results(w)

    def show_cities(self, w) :
        print('\n')
        t = dynamic_table(city_fields, file=self.log_file, console=self.args.console)
        for c in w.cities :
            t.add_line(c)

    def plot_results(self, w) :
        from_, to = w.get_interesting()
        p = plotter()
        title = f'Population {w.population} Infectiousness {w.get_infectiousness()} Auto-Immunity {w.get_auto_immunity()}'
        title += f'\nMax Days to Double {w.days_to_double:.1f}'
        x = w.get_days()[from_:to]
        data = [ {'label': var_to_title(v),
                  'data' : w.get_data(v)[from_:to] } for v in ('total_infected', 'infected') ]
        plotfile = f'{self.log_path}{self.log_filename}' if self.log_path else None
        p.plot(x, *data, title=title, file=plotfile, show=self.args.plot, format=self.args.format, props=self.props)

    def run_sensitivity(self):
        def one_col(w, name) :
            return float_to_str(w.params[name])
        s = f'repeat:1*{self.args.repeat}' if self.args.repeat else self.args.sensitivity
        sens = sensitivity(s)
        param_cols = [_f(var_to_title(n), '%10s',
                         functools.partial(one_col, name=n))
                      for n in sens.get_variables()]
        t = dynamic_table((param_cols+list(summary_fields)), file=self.log_file, console=self.args.console)
        if self.log_filename :
            detail_log = open(f'{self.log_path}{self.log_filename}-detail.log', 'w')
            self.write_log_header(detail_log)
            detail_table = dynamic_table((param_cols+list(log_fields)), file=detail_log)
            detail_log.write('\n')
        else :
            detail_log = detail_table = None
        params = []
        results = []
        for ss in sens :
            if False:
                params.append(ss)
                self.props.add_properties('\n'.join([f'{v[0]}={v[1]}' for v in ss]))
                cluster.make_cluster_info(self.props)
                w = world(props=self.props)
                w.run(logger=(lambda w: detail_table.add_line(w)) if detail_table else None)
                if detail_table :
                    detail_table.write()
                results.append(w)
                t.add_line(w)
            else :
                params.append(ss)
                self.props.add_properties('\n'.join([f'{v[0]}={v[1]}' for v in ss]))
                w = fast_world(props=self.props)
                w.run()
                results.append(w)
                w.params = { sss[0]:sss[1] for sss in ss }
                t.add_line(w)
        if self.args.plot or self.log_path:
            from_ = min([ w.get_interesting()[0] for w in results ])
            to = max([ w.get_interesting()[1] for w in results ])
            plot = plotter()
            x = range(from_, to)
            data = []
            colors = plot.make_colors(self.props)
            for p, w, c in zip(params, results, itertools.cycle(colors)) :
                d = w.get_data('total_infected')[from_:to]
                d1 = w.get_data('infected')[from_:to]
                while len(d) < len(x) :
                    d.append(d[-1])
                    d1.append(d1[-1])
                legend = ', '.join([ float_to_str(pp[1]) for pp in p ])
                data.append({ 'label':legend, 'data':d, 'color':c})
                data.append({ 'data': d1, 'style':'--', 'color': c})
            titles = []
            for t in ('population', 'infectiousness', 'auto_immunity') :
                if t not in sens.get_variables() :
                    titles.append(f'{var_to_title(t)} = {getattr(results[0], t)}')
            title = ' '.join(titles)
            if not self.args.repeat :
                title += '\nVarying {}'.format(', '.join([var_to_title(p) for p in sens.get_variables()]))
            plotfile = f'{self.log_path}{self.log_filename}' if self.log_path else None
            plot.plot(x, *data, title=title, legend=(not self.args.repeat), file=plotfile, show=self.args.plot,
                      format=self.args.format, props=self.props)
            if detail_log :
                detail_log.close()


epidemia().run()
