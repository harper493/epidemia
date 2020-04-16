import argparse

class argparser(object) :

    def __init__(self) :
        def arg(short, *args, **kwargs) :
            if short[0]=='-' :
                self.argnames.append(args[0][2:])
                p.add_argument(short, args[0], **kwargs)
            else :
                self.argnames.append(short)
                p.add_argument(short, **kwargs)
        self.argnames = []
        self.extra_props = {}
        self.props_file = None
        p = argparse.ArgumentParser()
        arg('stuff', nargs='*', help='property file or property assignments')
        arg('-c', '--console', action='store_true', help='send output to console')
        arg('-o', '--output', type=str, default=None, help='output file name')
        arg('-P', '--plot', action='store_true',  help='plot results as graph')
        arg('-X', '--profile', action='store_true',  help='run cProfile')
        arg('-v', '--verbose', action='store_true',  help="describe what's going on")
        arg('-V', '--very-verbose', action='store_true',  help="describe what's going on A LOT")
        arg('-n', '--initial', type=int, default=None, help='initial infected')
        arg('-i', '--infectiousness',type=float, default=None, \
                       help='level of infectiousness')
        arg('-a', '--auto-immunity', type=float, default=None, \
                       help='level of non-infectious immunity')
        a = p.parse_args()
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
            setattr(self, an, getattr(a, an))

if __name__=='__main__' :
    p = argparser()
    print(p.__dict__)