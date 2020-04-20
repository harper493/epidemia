import argparse
import sys

class argparser(object) :

    def __init__(self) :
        def arg(short, *args, **kwargs) :
            self.argnames.append(args[0][2:])
            long_ = args[0]
            if long_[-1]=='*' :
                long_ = long_[:-1]
            p.add_argument(short, long_, **kwargs)
        self.argnames = []
        self.command_line = ' '.join(sys.argv)
        self.extra_props = {}
        self.props_file = None
        p = argparse.ArgumentParser()
        p.add_argument('stuff', nargs='*', help='property file or property assignments')
        arg('-c', '--console', action='store_true', help='send output to console')
        arg('-C', '--city_count*', type=int, help='number of cities')
        arg('-R', '--repeat', type=int, default=0, help='run repeatedly with the same parameters')
        arg('-o', '--output', type=str, default=None, help='output file name')
        arg('-P', '--plot', action='store_true',  help='plot results as graph')
        arg('-X', '--profile', action='store_true',  help='run cProfile')
        arg('-v', '--verbose', action='store_true',  help="describe what's going on")
        arg('-V', '--very-verbose', action='store_true',  help="describe what's going on A LOT")
        arg('-n', '--initial_infected*', type=int, default=None, help='initial infected')
        arg('-p', '--population*', type=int, default=None, help='size of population')
        arg('-N', '--infected_cities*', type=str, default=None,
                       help='number or proportion of cities to be initially infected')
        arg('-i', '--infectiousness*',type=float, default=None, \
                       help='level of infectiousness (0-1)')
        arg('-S', '--sensitivity', type=str, default=None, help='perform sensitivity analysis')
        arg('-L', '--log-path', type=str, default=None, help='path for writing log files')
        arg('-a', '--auto-immunity*', type=float, default=None, \
                       help='level of non-infectious immunity (0-1)')
        a = p.parse_args()
        if a.sensitivity and a.repeat :
            self.error("Can't combine --sensitivity and --repeat in a single command")
        if not a.output :
            a.console = True
        if a.very_verbose :
            a.verbose = True
        for s in a.stuff :
            if '=' in s :
                ss = s.split('=')
                self.extra_props[ss[0]] = ss[1]
            else :
                self.props_file = s
        for an in self.argnames :
            an = an.replace('-', '_')
            if an[-1]=='*' :
                an = an[:-1]
                v = getattr(a, an)
                if v is not None:
                    self.extra_props[an] = v
            else :
                setattr(self, an, getattr(a, an))

    def error(self, text) :
        print(text)
        sys.exit(1)


if __name__=='__main__' :
    p = argparser()
    print(p.__dict__)