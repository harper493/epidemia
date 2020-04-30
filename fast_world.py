import subprocess
import csv
from infection_counter import infection_counter
from datetime import datetime
from math import *

name_trans = { 'total' : 'total_infected' }

class fast_world(infection_counter):

    def __init__(self, props):
        self.props = props
        self.population = props.get(int, 'population')
        self.daily = {}

    def run(self):
        temp_file = f"_tmp_{datetime.now().strftime('%Y%m%dT%H%M%S_%f')[:-3]}"
        csv_file = temp_file + ".csv"
        props_file = temp_file + ".props"
        with open(props_file,'w') as f:
            f.write(self.props.dump())
        subprocess.call(("./epidemia_fast", "--csv", "-o", csv_file, props_file))
        with open(csv_file, 'r') as infile:
            reader = csv.DictReader(infile)
            for row in reader:
                day = int(row['day'])
                self.daily[day] = { name_trans.get(n, n):(float(v) if '.' in v else int(v)) for n,v in row.items() }

    def get_days(self):
        return [ k for k in self.daily.keys() ]

    def get_data(self, name):
        return [ d[name] for d in self.daily.values() ]

    def get_data_point(self, name, day):
        return self.daily[day][name]

    def get_interesting(self):
        from_ = None
        to = len(self.daily)
        self.highest_day = max([ d["infected"] for d in self.daily.values() ])
        self.max_growth =  max([ d["growth"] for d in self.daily.values() ])
        if self.max_growth > 1 :
            self.days_to_double = log(2) / log(self.max_growth)
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


        
