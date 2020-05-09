import subprocess
import os
import csv
from infection_counter import infection_counter
from datetime import datetime
from math import *
from dataclasses import dataclass
from geometry import *
from copy import copy, deepcopy

name_trans = { 'total' : 'total_infected' }

class fast_world(infection_counter):

    @dataclass
    class _city_info():
        name: str
        index: int
        population: int
        location: point
        size: float
        infected: int = 0
        recovered: int = 0
        total: int = 0

    def __init__(self, props, args):
        self.props = props
        self.args = args
        self.population = props.get(int, 'population')
        self.auto_immunity = self.props.get(float, 'auto_immunity')
        self.infectiousness = self.props.get(float, 'infectiousness')
        self.daily = {}
        self.size = 100

    def run(self):
        temp_file = f"_tmp_{datetime.now().strftime('%Y%m%dT%H%M%S_%f')[:-3]}"
        csv_file = temp_file + ".csv"
        city_file = temp_file + '_cities.csv'
        props_file = temp_file + ".props"
        with open(props_file,'w') as f:
            f.write(self.props.dump())
        cmd_args = ["./epidemia_fast", "--csv", "--verbosity=0",
                          "-o", csv_file, "--city-data="+city_file ]
        if self.args.bubbles:
            cmd_args.append('--log-cities')
        if self.args.min_days:
            cmd_args.append(f'--min-days={self.args.min_days}')
        if self.args.max_days:
            cmd_args.append(f'--max-days={self.args.max_days}')
        if self.args.random:
            cmd_args.append(f'--random={self.args.random}')
        cmd_args.append(props_file)
        subprocess.call(cmd_args)
        with open(city_file, 'r') as cfile :
            city_reader = csv.DictReader(cfile)
            self.cities = {}
            self.cities_by_index = {}
            for row in city_reader:
                ci = fast_world._city_info(name=row['name'],
                                           index = int(row['index']),
                                           population=int(row['population']),
                                           location=point(string=row['location']),
                                           size=float(row['size']))
                self.cities[ci.name] = ci
                self.cities_by_index[ci.index] = ci
        self.max_infected = 0
        self.max_growth = 0
        self.highest_day = 0
        self.total_infected = 0
        today = None
        with open(csv_file, 'r') as infile:
            reader = csv.DictReader(infile)
            for row in reader:
                day = int(row['day'])
                city_no = int(row['city'])
                data = {name_trans.get(n, n): (float(v) if '.' in v else int(v)) for n, v in row.items()}
                if city_no and today:
                    city = today['cities'][self.cities_by_index[city_no].name]
                    infected = data['infected']
                    city.infected = infected
                    city.total =  data['total_infected']
                    city.recovered = city.total + data['immune'] - infected
                else:
                    today = data
                    today['cities'] = deepcopy(self.cities)
                    infected = today['infected']
                    if infected > self.max_infected:
                        self.max_infected = infected
                        self.highest_day = day
                    self.max_growth = max(self.max_growth, today['growth'])
                    self.total_infected = max(self.total_infected, today['total_infected'])
                    self.daily[day] = today
            self.days_to_double = log(2) / log(1 + self.max_growth/100) if self.max_growth else 0
        os.remove(city_file)
        os.remove(csv_file)
        os.remove(props_file)

    def get_days(self):
        return [ k for k in self.daily.keys() ]

    def get_data(self, name):
        return [ d[name] for d in self.daily.values() ]

    def get_data_point(self, name, day):
        return self.daily[day][name]

    def get_interesting(self):
        from_ = None
        to = len(self.daily)
        for d in self.daily.keys():
            ti = self.get_data_point('total_infected', d)
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


        
