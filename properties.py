#
# General properties file interface
#
# Reads a properties file in java format, e.g.:
#
# a.b.c.d    = abcd
# a.b.*.d    = abxd
#
# Provides a get function which takes a list of keys
# and returns the best entry, or None is nothing
# matches.
#
# The best entry is the one with the fewest wildcard
# elements. If there are multiple matching entries with the
# same number of them, it is undefined which is returned.
#
# The internal representation of the tree is a dict
# of dicts of dicts ... to the necessary depth. A leaf is
# represented as a string, unless there are also further
# keys at the same level. In that case it is represented
# as an entry with the key None.
#

import re
import codecs

class properties(object) :

    def __init__(self, *files) :
        self.root = {}
        for f in files :
            self.load_file(f)

#
# get - takes a list of keys and returns the best match
# for these keys as described above, or None if there
# is no match
#
# If the first parameter is a type, the value is converted accordingly.
#
    def get(self, *args) :
        result = None
        if isinstance(args[0], type) :
            type_ = args[0]
            args = args[1:]
        else :
            type_ = None
        d = self._get_prefixes(*args)
        if d :
            best = d[0][0]
            try :
                result = best[None] if isinstance(best, dict) else best
            except KeyError :
                pass
        if type_ :
            try :
                result = (type_)(result)
            except (ValueError, TypeError) :
                result = type_()
        return result
#
# translate - takes a list of keys and returns either the match
# which is found, or the last of the keys if there is none
#
    def translate(self, *args) :
        result = self.get(*args)
        if result is None :
            result = args[-1]
        return result

#
# get_all - get all keys and values matching the
# given prefix, as a dict keyed by key tuples, and values
#
    def get_all(self, *args) :
        result = {}
        def visitor(keys, value) :
            result[tuple(keys)] = value
        self.visit(visitor, *args)
        return result
        
#
# _get_prefixes - get the prefixes for a list of keys
#
# Returns a list of zero or more tuples, each containing:
#
# -- a dict or string to be further explored
# -- a list of the prefixes that were used to get this far
#

    def _get_prefixes(self, *args) :
        d = [(self.root, [])]
        for a in args :
            new_d = []
            if a=='*' :
                for dd in d:
                    if isinstance(dd[0], dict) :
                        for k in dd[0].keys() :
                            new_d.append((dd[0][k], dd[1]+[k]))
            else :
                for sfx in [a, '*'] :
                    for dd in d :
                        if isinstance(dd[0], dict) and sfx in dd[0] :
                            new_d.append((dd[0][sfx], dd[1]+[sfx]))
            d = new_d
        return d

#
# visit - apply a visitor function (keys, value) to all values
# for the given prefix. 'keys' is a list of the key elements.
#
# if best_only=True, apply only to the best match for the keys,
# otherwise apply to all wild variants
#
    def visit(self, fn, *args, **kwargs) :
        for key, value in self.iter(*args, **kwargs) :
            fn(key, value)
        return
#
# iter - return an iterator matching the requested keys
#
    def iter(self, *args, **kwargs) :
        best_only = 'best_only' in kwargs and kwargs['best_only']
        d = self._get_prefixes(*args)
        if d:
            if best_only :
                d = d[0:1]
            for dd in d :
                yield from self._iter_one(dd)

    def _iter_one(self, d) :
        if isinstance(d[0], str) :
            yield (d[1], d[0])
        else :
            for k in sorted(d[0]) :
                if k is None :
                    fn(d[1], d[0][None])
                else :
                    yield from self._iter_one((d[0][k], d[1] + [k]))

    def __iter__(self):
        yield from self.iter()
#
# dump - produce a string in properties file format of the
# specified prefix
#
    def dump(self, *args, **kwargs) :
        return '\n'.join(['%-40s = %s' % ('.'.join(keys), value) \
                          for keys, value in self.iter(*args, **kwargs)])

#
# add_properties - takes a string consisting of lines in 
# java properties format, and adds them to the tree. Deal with continuation lines flagged by a
# '\' as the last character of the preceding line.
#
    def add_properties(self, props) :
        rx = re.compile(r'^\s*(\S+)\s*=\s*(.+?)\s*(#.*)?$')
        line = ''
        for l in props.split('\n') :
            l = re.sub(r'\s+', ' ', l) # Make leading spaces single space
            if len(l) and l[-1]=='\\' :
                line += l[:-1]
                line = re.sub(r'\s+$', '', line) # Remove all trailing spaces
                continue
            else :
                line += l;
                if len(line) == 0:
                    continue
                if line[0] == '@':
                    self.do_directive(line)
                elif line[0] != '#':
                    m = rx.match(line)
                    if m :
                        pname, pvalue = m.group(1, 2)
                        text = re.sub(r"''", "'", pvalue)
                        self.add(pname, text)
                line = ''

#
# add - add a single entry to the tree
#
    def add(self, name, value) :
        keys = name.split('.')
        d = self.root
        for k in keys[:-1] :
            if k in d :
                if isinstance(d[k], dict) :
                    d = d[k]
                else :
                    d1 = { None : d[k] }
                    d[k] = d1
                    d = d1
            else :
                d[k] = {}
                d = d[k]
        kn = keys[-1]
        if kn in d and isinstance(d[kn], dict) :
            d[kn][None] = value
        else :
            d[kn] = value
#
# do_directive  process a directive line beginning with @
#

    def do_directive(self, line) :
        m = re.match(r'^@(\w+)(?:\s+(.*))?', line)
        if m :
            directive, args = m.group(1, 2)
            if directive=='include' :
                self.load_file(args)
# 
# load_file - load a property file
#
    def load_file(self, path) :
        f = codecs.open(path, encoding='utf-8')
        self.add_properties(f.read())
        f.close()



