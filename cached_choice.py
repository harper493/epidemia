import random
import bisect

class cached_choice(object) :

    def __init__(self, coll, key):
        self.coll = coll
        self.keys = [ key(c) for c in coll ]
        self.total_keys = sum(self.keys)
        cum = 0
        self.cum_keys = [ ]
        for k in self.keys[:-1] :
            cum += k / self.total_keys
            self.cum_keys.append(cum)

    def choose(self):
        r = random.random()
        i = bisect.bisect(self.cum_keys,r)
        return self.coll[i]

if __name__=='__main__' :
    c = ( 1, 2, 3, 4, 5)
    k = ( 0.1, 0.1, 0.5, 0.01, 1)
    print([c.index(cc) for cc in c])
    cc = cached_choice(c, lambda cc: k[c.index(cc)])
    print(cc.coll, cc.keys, cc.cum_keys, cc.total_keys)
    histo = {}
    for i in range(50000) :
        ch = cc.choose()
        try :
            histo[ch] += 1
        except KeyError :
            histo[ch] = 1
    sum_ = sum(histo.values())
    print(sum_, histo.keys(), histo.values(), [ histo[h]/sum_ for h in histo.keys() ])

