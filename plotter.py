import matplotlib.pyplot as plt
import os
from matplotlib import animation
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
from matplotlib.patches import Rectangle
import numpy as np
from utility import *
from dataclasses import dataclass
import time
from utility import *
from constructor import constructor

default_x_size = 12
default_y_size = 8
initial_x_limit = 200
x_increase_delta = 100
initial_y_min = 100

@dataclass
class line_info():
    element: str
    color: str = None
    style: str = None

animated_formats = ('mp4', 'gif', 'html5')

class plotter():

    class range_maker():
        """
        range_maker - iterator to feed to the animation class

        Each call yields: (world_no, day_no, last_day, last_world)

        We need to have look ahead to know whether this is the last day of this
        world, and the last day of all worlds, so we can save files using the
        animation. It makes for very messy code, since the logic essentially
        has to be repeated twice.
        """

        def __init__(self, plotter):
            self.plotter = plotter
            self.worlds = plotter.worlds
            self.world_no = 0
            self.highest_day = 0
            self.last_signalled = -1

        def _get_next(self):
            daily = None
            while True:
                w = self.worlds[self.world_no]
                if w:
                    d = self._get_day(self.highest_day + 1, wait=False)
                    if d:
                        daily = d
                        self.highest_day += 1
                        if self.plotter.incremental:
                            break
                    elif daily:
                        break
                    else:
                        time.sleep(0.1)
            return daily

        def _get_day(self, day, wait):
            while True:
                w = self.worlds[self.world_no]
                if w:
                    d = w.get_daily(self.highest_day + 1)
                    if d or not wait:
                        break
                    else:
                        time.sleep(0.1)
            return d

        def __iter__(self):
            while self.world_no >= 0 and self.world_no < len(self.worlds):
                daily = self._get_next()
                if daily.infected < 0:
                    if self.last_signalled + 1 < self.highest_day:
                        self.last_signalled = self.highest_day - 1
                        last_world = self.world_no + 1 >= len(self.worlds)
                        wno = self.world_no
                        self.world_no += 1
                        self.highest_day = 0
                        yield (wno, self.last_signalled, True, last_world)
                    else:
                        self.world_no += 1
                        self.highest_day = 0
                    if self.world_no >= len(self.worlds):
                        yield(-1, -1, True, True)
                        return
                else:
                    next_daily = self._get_day(self.highest_day + 1, wait=True)
                    self.last_signalled = self.highest_day
                    last_day = next_daily.infected<0
                    last_world = last_day and self.world_no + 1 >= len(self.worlds)
                    yield (self.world_no, self.highest_day, last_day, last_world)

    @dataclass
    class colors():
        background = 'ivory'
        button = 'linen'
        button_hover = 'bisque'
        susceptible = 'deepskyblue'
        infected = 'red'
        gestating = 'lightsalmon'
        asymptomatic = 'tomato'
        recovered = 'mediumseagreen'
        immune = 'limegreen'
        vaccinated = 'chartreuse'
        dead = 'black'
        total = 'cornflowerblue'
        graph_infected = 'darkorange'
        graph_total = 'red'
        graph = 'blue,forestgreen,red,cyan,darkviolet,goldenrod,' \
                'sienna,cornflowerblue,lime,lightcoral,turquoise,orange,magenta,' \
                'purple,yellowgreen,rosybrown,steelblue,crimson,black'
        table_header_background = 'oldlace'
        table_cell_background = 'floralwhite'

        def load(self, props):
            for pname, pvalue in props:
                if pname[0] == 'color':
                    setattr(self, pname[-1], pvalue)
            self.graph = self.graph.split(',')

    arg_table = {
        'title' : (str, ''),
        'log_scale' : (bool, True),
        'legend': (bool, True),
        'file': (str, ''),
        'show': (bool, False),
        'format': (str, ''),
        'props': None,
        'total_color': None,
        'infected_color': None,
        'lines': None,
        'incremental': False,
        'size': None,
        'x_size': default_x_size,
        'y_size': default_y_size,
        'left_margin': 0.06,
        'right_margin': 0.06,
        'bottom_margin': 0.07,
        'graph_height': 0.8,
        'surtitle_left_margin': 0.1,
        'surtitle_base': 1.02,
        'surtitle_line_height': 0.035,
        'surtitle_width': 0.08,
        'surtitle_font_size': 9,
        'table_height': 0.25,
        'table_width': 0.8,
        'table_top_margin': 0.03,
        'table_font_size': 9,
        'display': True,
        'square': False,
        'surtitle': [],    # must be a list of 2-tuples (label, value)
        'table': None,
        'save_frames': None,
        'file': None,
        'format': None,
        'mp4_frame_rate': 8,
    }

    def __init__(self, **kwargs):
        constructor(plotter.arg_table, args=kwargs).apply(self)
        if self.lines is None:
            self.lines = (line_info('total', style='solid'),
                          line_info('infected', style='dashed'),
                          line_info('dead', style='dotted'))
        self.colors = plotter.colors()
        self.colors.load(self.props)

    @staticmethod
    def make_line_info(*args, **kwargs):
        return line_info(*args, **kwargs)

    def plot(self, worlds, labels, show=True):
        self.pre_plot(x_size=default_x_size, y_size=default_y_size)
        self.worlds = worlds
        self.labels = labels
        self.last_day = 0
        self.last_world = 0
        self.frame_no = 0
        self.build()
        self.make_animation()
        if self.file and self.format in animated_formats:
                self.save_animation()
        if show:
            plt.show()

    def build(self):
        size = self.props.get(int, 'plot', 'size') or self.size
        x_size = self.props.get(int, 'plot', 'x_size') or self.size or self.x_size
        if self.square:
            y_size = x_size
        else:
            y_size = self.props.get(int, 'plot', 'y_size') or self.size or self.y_size
        self.fig = plt.figure(figsize=(x_size, y_size))
        self.fig.patch.set_facecolor(self.colors.background)
        if self.table:
            bottom = self.bottom_margin + self.table_height + self.table_top_margin
            height = self.graph_height - self.table_height - self.table_top_margin
        else:
            bottom = self.bottom_margin
            height = self.graph_height
        self.graph = self.fig.add_axes((self.left_margin, bottom,
                                        (1 - self.left_margin - self.right_margin), height))
        self.x_values = [ d for d in range(1, initial_x_limit) ]
        self.graph.set_xlim(0, len(self.x_values))
        self.graph.set_xlabel('Days')
        self.graph.set_ylabel('People')
        self.values, self.graph_lines = [], []
        if self.title :
            self.fig.suptitle(self.title, fontsize=10)
        if self.log_scale :
            self.graph.set_yscale('log')
        for lab, color in zip(self.labels, self.colors.graph):
            self.values.append({ l.element : [np.nan] * len(self.x_values) for l in self.lines})
            self.graph_lines.append({ l.element : self.graph.plot(self.x_values, self.values[-1][l.element],
                                                                  color=l.color or color,
                                                                  label=lab if l.style=='solid' else None,
                                                                  linestyle=l.style or 'solid')[0]
                                      for l in self.lines})
            legend2 = self.graph.legend([ g for g in self.graph_lines[0].values() ],
                                        [ n for n in self.graph_lines[0].keys()],
                                        loc='center left' if self.legend else 'upper left')
            self.graph.add_artist(legend2)
            if self.legend:
                self.graph.legend(loc='upper left')
        self.first = True
        self.top_ax = self.graph
        self.build_extra()
        self.build_surtitle()
        self.build_table()

    def build_surtitle(self):
        row_labels = [ s[0] for s in self.surtitle ]
        col_values = [ [ s[1] ] for s in self.surtitle ]
        self.surtitles = self.top_ax.table(cellText=col_values, rowLabels=row_labels,
                                           fontsize=self.surtitle_font_size, edges='open',
                                           bbox=(self.surtitle_left_margin, self.surtitle_base,
                                                 self.surtitle_width,
                                                 self.surtitle_line_height * len(row_labels)))

    def build_table(self):
        if self.table:
            col_labels = [ f.text for f in self.table ]
            col_colors = [ self.colors.table_header_background ] * len(col_labels)
            data = [ [ '' ] * len(col_labels)] * len (self.labels)
            cell_colors = [ [ self.colors.table_cell_background ] * len(col_labels)] * len (self.labels)
            box = (self.left_margin,
                   -(self.table_height + self.table_top_margin)*2,
                   self.table_width, self.table_height*1.5)
            self.table_obj = self.graph.table(colLabels=col_labels, colColours=col_colors,
                                              cellColours=cell_colors,
                                              fontsize=self.table_font_size, edges='horizontal',
                                              bbox=box, zorder=2)
            for pos, c in self.table_obj.get_celld().items():
                c.set_facecolor(self.colors.table_cell_background)
                c.set_text_props(ha='right')
                if pos[0]==0:
                    c.set_text_props(weight='bold')
                elif pos[1]==0:
                    c.set_text_props(color=self.colors.graph[pos[0]-1])

    def update_table(self, w, row):
        if self.table:
            for col, f in enumerate(self.table):
                self.table_obj[row, col].set_text_props(text=f(w), fontsize=self.table_font_size)

    def build_extra(self):
        pass

    def pre_plot(self, x_size, y_size):
        pass

    def make_animation(self):
        self.animation = FuncAnimation(self.fig, self.do_day,
                                       frames=plotter.range_maker(self),
                                       blit=False,
                                       repeat=False)

    def reset(self):
        for w in range(len(self.labels)):
            for l in self.lines:
                e = l.element
                for i in range(len(self.values[w][e])):
                    self.values[w][e][i] = np.nan

    def do_day(self, r):
        world_no, day, last_day, last_world = r
        if world_no != self.last_world:
            self.last_world = world_no
            self.last_day = 0
        if world_no >= 0:
            w = self.worlds[world_no]
            while self.last_day < day:
                self.last_day += 1
                daily = w.get_daily(self.last_day)
                self.day_pre_extra(self.last_day, w, daily)
                if self.last_day > len(self.x_values) :
                    self.extend_x()
                if self.first:
                    self.first = False
                    self.graph.set_ylim(initial_y_min, w.population)
                for l in self.lines:
                    e = l.element
                    self.values[world_no][e][self.last_day-1] = getattr(daily, e)
                    self.graph_lines[world_no][e].set_ydata(self.values[world_no][e])
                self.day_extra(self.last_day, w, daily)
            if self.save_frames:
                fn = f'{self.save_frames}-{self.frame_no:04d}.png'
                self.frame_no += 1
                plt.savefig(fn, bbox_inches='tight')
            if last_day:
                self.update_table(self.worlds[self.last_world], self.last_world + 1)
            if self.file and last_world:
                self.save_file()

    def day_pre_extra(self, day, world, daily):
        pass

    def day_extra(self, day, world, daily):
        pass

    def next_is_end(self, w, day):
        result = False
        next_daily = w.get_daily(day)
        if next_daily and next_daily.infected < 0 :
            result = True
        return result

    def extend_x(self):
        old_x = len(self.x_values)
        new_x = old_x + x_increase_delta
        self.x_values += [ d for d in range(old_x, new_x) ]
        self.graph.set_xlim(0, len(self.x_values))
        for w in range(len(self.values)):
            for l in self.lines:
                e = l.element
                self.values[w][e] += [ np.nan ] * x_increase_delta
                self.graph_lines[w][e].set_xdata(self.x_values)
                self.graph_lines[w][e].set_ydata(self.values[w][e])

    def save_file(self):
        if self.format not in animated_formats:
            format = self.format or 'png'
            f = self.file
            if '.' not in os.path.basename(self.file) :
                f = f'{self.file}.{self.format}'
            plt.savefig(f, bbox_inches='tight', format=format)

    def save_animation(self):
        format = self.format or 'mp4'
        file = self.file
        if '.' not in os.path.basename(file) :
            file = f'{file}.{format}'
        if format == 'mp4':
            Writer = animation.writers['ffmpeg']
            writer = Writer(fps=self.mp4_frame_rate)
            self.animation.save(file, writer=writer)
        elif format == 'gif':
            self.animation.save(file, writer='imagemagick', fps=self.mp4_frame_rate)
        elif format == 'html5':
            with open(file, 'w') as f:
                f.write(self.animation.to_html5_video())

