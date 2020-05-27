#!/usr/bin/python3.8

from world import world
from properties import properties
from cluster import cluster
from sensitivity import sensitivity
from utility import *
from dynamic_table import dynamic_table
from plotter import plotter, line_info
from argparser import argparser
from math import *
from datetime import datetime
from fast_world import fast_world
from bubbles import bubbles
import functools
import itertools
import numpy as np
from threading import Lock, Thread
import time
import os

import cProfile

bubbles_async = True

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
    _f('max_infected', '%7d'),
    _f('%', "%5.2f", lambda w: 100 * w.max_infected / w.population),
    _f('total', '%7d'),
    _f('%', "%5.2f", lambda w: 100 * w.total / w.population),
    _f('dead', '%7d'),
    _f('%', '%5.2f', lambda w: 100 * w.dead / w.population),
    _f('days_to_double', '%5.1f'),
    _f('Days to Peak', '%3d', lambda w: w.highest_day),
)

sensitivity_lines = (
    line_info('total', style='solid'),
    line_info('infected', style='dashed'),
)

class epidemia() :
    
    def __init__(self):
        self.args = argparser()
        propfiles = ['base.props']
        if self.args.props_file :
            propfiles.append(self.args.props_file)
        self.props = properties(*propfiles, cmd_args=self.args.extra_props)
        cluster.make_cluster_info(self.props)
        self.bubble_thread = None
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
        w = fast_world(props=self.props, args=self.args)
        w.run(sync=False)
        if self.args.bubbles:
            self.plot_bubbles(w)
        elif self.args.plot :
            w.terminate()
            self.plot_results(w)
        w.terminate()

    def show_cities(self, w) :
        print('\n')
        t = dynamic_table(city_fields, file=self.log_file, console=self.args.console)
        for c in w.cities :
            t.add_line(c)

    def plot_results(self, w) :
        from_, to = w.get_interesting()
        surtitle = [('Population', w.population), ('Infectiousness', w.get_infectiousness() ),
                    ('Auto-Immunity', w.get_auto_immunity()),
                    ('Max Days to Double', f'{w.days_to_double:.1f}')]
        plotfile = f'{self.log_path}{self.log_filename}' if self.log_path else None
        p = plotter(surtitle=surtitle, file=plotfile, show=self.args.plot, format=self.args.format, props=self.props,
                    legend=False)
        x = w.get_days()[from_:to]
        data = [ {'label': var_to_title(v),
                  'data' : w.get_data(v)[from_:to] } for v in ('total', 'infected') ]
        p.plot([w], ('total', 'infected'))

    def plot_bubbles(self, w):
        surtitle = [('Population', w.population),
                    ('Infectiousness', w.get_infectiousness()),
                    ('Auto Immunity', w.get_auto_immunity())]
        plotfile = f'{self.log_path}{self.log_filename}' if self.log_path else None
        rd = bubbles(file=plotfile, format=self.args.format, props=self.props, world_size=w.size,
                     population=w.population, legend=False, incremental=True, surtitle=surtitle,
                     save_frames=self.args.save_frames, log_scale=not self.args.linear)
        rd.plot([w], ('total', 'infected'), show=not self.args.no_display)

    def run_sensitivity(self):
        def one_col(w, name) :
            return float_to_str(w.params[name])
        s = f'repeat:1*{self.args.repeat}' if self.args.repeat else self.args.sensitivity
        if self.args.random==0:
            self.args.random = true_random()
        sens = sensitivity(s)
        self.ranges = [ r for r in sens ]
        param_cols = [_f(var_to_title(n), '%10s',
                         functools.partial(one_col, name=n))
                      for n in sens.get_variables()]
        table_cols = param_cols+list(summary_fields)
        self.table = dynamic_table(table_cols, file=self.log_file, console=self.args.console)
        if self.log_filename :
            detail_log = open(f'{self.log_path}{self.log_filename}-detail.log', 'w')
            self.write_log_header(detail_log)
            detail_table = dynamic_table((param_cols+list(log_fields)), file=detail_log)
            detail_log.write('\n')
        else :
            detail_log = detail_table = None
        self.worlds = []
        if not self.args.no_base:
            self.base_params = [ (f'base.{r[0]}', r[1]) for r in self.ranges[0] ]
        else:
            self.base_params = []
        self.run_thread = Thread(target=self._world_runner)
        self.run_thread.start()
        if self.args.plot or self.log_path:
            self.labels = [ ', '.join([ float_to_str(pp[1]) for pp in p ]) for p in self.ranges ]
            surtitle = [ (f'{var_to_title(t)}', f'{float_to_str(self.props.get(float, t))}')
                       for t in ('population', 'infectiousness', 'auto_immunity')
                       if t not in sens.get_variables() ]
            if not self.args.repeat :
                title = '\nVarying: {}'.format(', '.join([var_to_title(p) for p in sens.get_variables()]))
            plotfile = f'{self.log_path}{self.log_filename}' if self.log_path else None
            plot = plotter(title=title, legend=(not self.args.repeat), file=plotfile, show=self.args.plot,
                           format=self.args.format or 'svg', props=self.props, incremental=False, surtitle=surtitle,
                           table=table_cols, save_frames=self.args.save_frames,
                           lines=sensitivity_lines, log_scale=not self.args.linear)
            plot.plot(self.worlds, self.labels, show=self.args.plot)
        if detail_log :
            detail_log.close()
        self.run_thread.join()

    def _world_runner(self):
        for ss in self.ranges:
            self.props.add_properties(values=ss)
            self.props.add_properties(values=self.base_params)
            w = fast_world(props=self.props, args=self.args)
            w.params = {sss[0]: sss[1] for sss in ss}
            self.worlds.append(w)
            w.run(sync=True)
            if self.table:
                self.table.add_line(w)

epidemia().run()
