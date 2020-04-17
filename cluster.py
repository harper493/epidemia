from utility import *
from reciprocal import reciprocal
from infection_counter import infection_counter
import random
from functools import partial

from cached_choice import cached_choice

class cluster(infection_counter) :

    cluster_id = 1

    def __init__(self, name, city_, world_, size: int=0, depth: int=0):
        self.name, self.city, self.world_, self.size = name, city_, world_, size
        infection_counter.__init__(self)
        self.depth = depth
        self.pop = 0
        self.people = set()
        self.members = []
        self.parent = None
        self.exposure = 0
        self.exposure_good = False
        self.location = self.city.get_random_location()
        #print(str(self))

    def __str__(self):
        return 'cluster %s city %s loc %s depth %d pop %d' % \
               (self.name, self.city.name, str(self.location), self.depth, self.pop)

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
        self.exposure_good = False

    def expose(self):
        inf = self.world_.get_infectiousness()
        self.exposure += inf
        p = self.parent
        while p :
            inf *= self.world_.get_cluster_exposure()
            p.exposure += inf
            p = p.parent

    def get_exposure(self):
        if not self.exposure_good :
            p = self.parent
            while p :
                self.exposure += p.exposure
                p = p.parent
            self.exposure_good = True
        return self.exposure

    def enroll(self, p: 'person'):
        assert(p not in self.people)
        self.pop += 1
        self.people.add(p)

    @staticmethod
    def make_clusters(world_, obj) :
        obj.clusters = {}
        for cname, cprops in cluster.cluster_info.items() :
            obj.clusters[cname] = { 0:[] }
            min_pop = cprops['min_pop']
            max_pop = min(cprops['max_pop'], obj.pop * cprops['max_proportion'])
            avg_pop = cprops['average_pop']
            count = obj.pop // avg_pop
            obj.clusters[cname][0] = cluster_collection(f'{obj.name}.{cname}', obj, 0, count, min_pop, max_pop, obj.pop)
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
                d += 1
                obj.clusters[cname][d] = cluster_collection(f'{obj.name}.{cname}', obj, d, count, nest_min, max_, sum_)

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
                            cc.parent = pcs.choose()
                            cc.parent.members.append(cc)
                        busy = True
            depth += 1
        busy, d = True, 0
        while busy :
            busy = False
            for city in world_.cities :
                for cname, cl in city.clusters.items() :
                    if d <= max(cl.keys()) :
                        busy = True
                        for cl2 in cl[d] :
                            if cl2.parent :
                                cl2.parent.pop += cl2.pop
                                cl2.parent.people.update(cl2.people)
                        cl[d].rebuild_cache()
            d += 1

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

class cluster_collection(object) :

    def __init__(self, name, city_, depth, count=0, min_pop=0, max_pop=0, total=0):
        self.name = name
        self.city_, self.depth = city_, depth
        self.count, self.min_pop, self.max_pop, self.total = count, min_pop, max_pop, total
        self.world_ = self.city_.world_
        self.content = {}
        self.cache = None
        if self.count:
            self.build()

    def build(self, count=0, min_pop=0, max_pop=0, total=0):
        sizes = reciprocal(count or self.count, min_pop or self.min_pop, max_pop or self.max_pop, total or self.total).get()
        self.content = [cluster(f'{self.name}.{self.depth}.{i}', self.city_, self.world_, size=s, depth=self.depth) \
                                  for i,s in enumerate(sizes) ]
        self.rebuild_cache()

    def rebuild_cache(self):
        self.cache = cached_choice(self.content, lambda c: c.size)

    def choose(self):
        return self.cache.choose()

    def __iter__(self):
        for cl in self.content :
            yield cl


