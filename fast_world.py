import subprocess
import os
import csv
from infection_counter import infection_counter
from datetime import datetime
from math import *
from dataclasses import dataclass
import dataclasses
from geometry import *
from copy import copy, deepcopy
from threading import Lock, Thread
from file_listener import file_listener
from csv_listener import csv_listener
import time

days_to_double_start = 10
default_world_size = 100

class fast_world(infection_counter):

    @dataclass
    class _city_info():
        name: str
        index: int
        population: int
        location: point
        size: float
        
        fields = None

        @staticmethod
        def get_fields():
            if fast_world._city_info.fields is None:
                fast_world._city_info.fields = [ f.name for f in dataclasses.fields(fast_world._city_info) ]
            return fast_world._city_info.fields

    @dataclass
    class _day_info():
        city: int
        gestating: int = 0
        infected: int = 0
        recovered: int = 0
        total: int = 0
        growth: float = 0
        immune: int = 0
        asymptomatic: int = 0
        vaccinated: int = 0
        dead: int = 0
        susceptible: int = 0
        severe: int = 0
        population: int = 0
        days_to_double: float = 0

        fields = None

        @staticmethod
        def get_fields():
            if fast_world._day_info.fields is None:
                fast_world._day_info.fields = [ f.name for f in dataclasses.fields(fast_world._day_info) ]
            return fast_world._day_info.fields

    def __init__(self, props, args):
        self.props = props
        self.args = args
        self.population = props.get(int, 'population')
        self.auto_immunity = self.props.get(float, 'auto_immunity')
        self.infectiousness = self.props.get(float, 'infectiousness')
        self.daily = {}
        self.daily_lock = Lock()
        self.size = props.get(int, 'world_size') or default_world_size
        self.cities = {}
        self.cities_by_index = {}
        self.total = 0
        self.dead = 0
        self.max_infected = 0
        self.max_growth = 0
        self.highest_day = 0
        self.days_to_double = 0
        self.today = None

    def run(self, sync=True):
        os.system("mkdir -p tmp")
        temp_file = f"tmp/_tmp_{datetime.now().strftime('%Y%m%dT%H%M%S_%f')[:-3]}"
        self.csv_file = temp_file + ".csv"
        self.city_file = temp_file + '_cities.csv'
        self.props_file = temp_file + ".props"
        with open(self.props_file,'w') as f:
            f.write(self.props.dump())
        cmd_args = ["./epidemia_fast", "--csv", "--verbosity=0",
                          "-o", self.csv_file, "--city-data=" + self.city_file ]
        if self.args.bubbles:
            cmd_args.append('--log-cities')
        if self.args.min_days:
            cmd_args.append(f'--min-days={self.args.min_days}')
        if self.args.max_days:
            cmd_args.append(f'--max-days={self.args.max_days}')
        if self.args.double:
            cmd_args.append('--double')
        if self.args.random:
            cmd_args.append(f'--random={self.args.random}')
        cmd_args.append(self.props_file)
        self.process = subprocess.Popen(cmd_args)
        self.city_listener = file_listener(self.city_file)
        self.city_reader = csv_listener(self.city_listener, self._unpack_city)
        self.city_listener.join()
        self.city_reader.join()
        self.csv_listener = file_listener(self.csv_file)
        self.csv_reader = csv_listener(self.csv_listener, self._unpack_row)
        if sync:
            self.terminate()

    def terminate(self):
        self.process.wait()
        self.csv_reader.join()
        self.csv_listener.join()
        os.remove(self.city_file)
        os.remove(self.csv_file)
        os.remove(self.props_file)

    def _unpack_city(self, row):
        if row:
            ci = fast_world._city_info(name=row['name'],
                                   index=int(row['index']),
                                   population=int(row['population']),
                                   location=point(string=row['location']),
                                   size=float(row['size']))
            self.cities[ci.name] = ci
            self.cities_by_index[ci.index] = ci

    def _unpack_row(self, row):
        if row:
            day = int(row['day'])
            city_no = int(row['city'])
            data = fast_world._day_info(**{ n: (float(v) if '.' in v else int(v)) for n, v in row.items()
                                             if n in fast_world._day_info.get_fields() })
            if data.city==0:
                self.today = data
                data.population = self.population
                data.susceptible = self.population - data.total - data.immune
                self.today.cities = {}
                if data.infected > self.max_infected:
                    self.max_infected = data.infected
                    self.highest_day = day
                self.dead = data.dead
                self.max_growth = max(self.max_growth, data.growth)
                self.days_to_double = max(self.days_to_double, data.days_to_double)
                self.total = data.total
                with self.daily_lock:
                    #print('$$$', self, day, max(self.daily.keys() or [-1]), self.today)
                    self.daily[day] = self.today
            else:
                city = self.cities_by_index[city_no]
                data.population = city.population
                data.susceptible = city.population - data.total - data.immune
                self.today.cities[data.city] = data
        else :
            with self.daily_lock:
                self.daily[max(self.daily.keys()) + 1] = fast_world._day_info(city=0, infected=-1)

    def get_days(self):
        return [ k for k in self.daily.keys() ]

    def get_daily(self, day):
        try:
            with self.daily_lock:
                return self.daily[day]
        except KeyError:
            return None

    def get_data(self, name):
        return [  getattr(d, name) for d in self.daily.values() ]

    def get_data_point(self, name, day):
        return getattr(self.daily[day], name)

    def get_interesting(self):
        from_ = None
        to = len(self.daily)
        for d in self.daily.keys():
            ti = self.get_data_point('total', d)
            if from_:
                if d > self.highest_day and self.get_data_point('infected', d) < ti // 5:
                    to = d
                    break
            elif ti > sqrt(self.population):
                from_ = d
        return(from_ or 0, to)

    def get_auto_immunity(self):
        return self.props.get(float, 'auto_immunity')

    def get_infectiousness(self):
        return self.props.get(float, 'infectiousness')


        
