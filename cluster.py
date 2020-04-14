from utility import *
from reciprocal import reciprocal
import random
from functools import partial

class cluster(object) :

    cluster_id = 1

    def __init__(self, name, type_, city_, world_, pop: int=0, size: int=0, depth: int=0):
        self.name, self.type_, self.city, self.pop, self.size = name, type_, city_, pop, size
        self.depth = depth
        self.members = []
        self.parent = None
        self.exposure = 0
        self.location = self.city.get_random_location()
        #print(str(self))

    def __str__(self):
        return 'cluster %s type %s city %s loc %s depth %d pop %d' % \
               (self.name, self.type_, self.city.name, str(self.location), self.depth, self.pop)

    def show_detail(self) :
        indent = 0
        c = self
        result = []
        while c :
            result.append('  '*indent + str(c))
            indent += 1
            c = c.parent
        return '\n'.join(result)


    def reset(self):
        self.exposure = 0

    @staticmethod
    def make_clusters(world_, obj) :
        obj.clusters = {}
        for cname, cprops in cluster.cluster_info.items() :
            obj.clusters[cname] = { 0:[] }
            min_pop = cprops['min_pop']
            max_pop = min(cprops['max_pop'], obj.pop * cprops['max_proportion'])
            avg_pop = cprops['average_pop']
            count = obj.pop // avg_pop
            sizes = reciprocal(count , min_pop, max_pop, obj.pop).get()
            obj.clusters[cname][0] = [ cluster(cluster._get_cluster_id(), cname, obj, world_, pop=s) \
                                       for s in sizes ]
            depth = cprops['depth']
            nest_avg = cprops['nest_average']
            nest_min = cprops['nest_min']
            nest_max = cprops['nest_max']
            d = 0
            dpop = avg_pop
            while (depth==0 or d<depth) and count // nest_avg > cprops['min_nested_cluster_count'] :
                dpop *= nest_avg
                count = count // nest_avg
                sum_ = count*nest_avg
                max_ = min(nest_max, sum_ - count * nest_min)
                sizes = reciprocal(count, nest_min, max_, sum_).get()
                d += 1
                obj.clusters[cname][d] = [ cluster(d*100000+cluster._get_cluster_id(), cname, obj, world_, depth=d, size=s)
                                                  for s in sizes ]

    @staticmethod
    def nest_clusters(world_):
        busy = True
        depth = 0
        while busy :
            busy = False
            for city in world_.cities :
                for cname, cl in city.clusters.items() :
                    same_city = world_.props.get(float, 'cluster', cname, 'same_city')
                    if (depth+1) in cl :
                        for cc in cl[depth] :
                            r= random.random()
                            pcs = cl[depth + 1]
                            if r > same_city :
                                others = [ cc for cc in world_.cities \
                                           if city is not cc and cc.clusters[cname].get(depth+1, None) ]
                                if others :
                                    pc = get_random_member(others, lambda cc: 1/city.distance(cc))
                                    pcs = pc.clusters[cname][depth+1]
                            cc.parent = get_random_member(pcs, lambda cl: cl.size)
                            cc.parent.members.append(cc)
                        busy = True
            depth += 1
        for city in world_.cities :
            for cname, cl in city.clusters.items() :
                for d in sorted(cl.keys())[1:] :
                    for cl2 in cl[d] :
                        cl2.pop = sum([ cl3.pop for cl3 in cl2.members ])

    @staticmethod
    def make_cluster_info(props) :
        cluster.cluster_info = {}
        cluster.info_defaults = {}
        props.visit(partial(cluster.make_one_cluster, props), 'cluster')
        for dname, dvalue in cluster.info_defaults.items() :
            for c in cluster.cluster_info.values() :
                if dname not in c :
                    c[dname] = dvalue

    @staticmethod
    def make_one_cluster(props, pname, cvalue) :
        cname = pname[1]
        if cname=='*' :
            cluster.info_defaults[pname[2]] = number(cvalue)
        else :
            if cname not in cluster.cluster_info :
                cluster.cluster_info[cname] = {}
            cluster.cluster_info[cname][pname[2]] = number(cvalue)

    @staticmethod
    def _get_cluster_id() :
        result = cluster.cluster_id
        cluster.cluster_id += 1
        return result

