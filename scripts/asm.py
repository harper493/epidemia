#!/usr/bin/env python

import re
import sys
import os

filename = sys.argv[1]
filepath = 'asm/' + filename + '.s'

try :
    os.remove(filepath)
    foo = 1
except : pass

os.system('make ' + filepath)

f = open(filepath)
lines = f.readlines()
f.close()

FILE_RE=re.compile(r"\s+\.file (\d+) \"(.*)\"")
LOC_RE =re.compile(r"\s+\.loc (\d+) (\d+)")
LABELS_RE = re.compile(r"\.LBE.*|\.LBB.*|\.LVL.*")

output = []
files = {}
filenames = {}
for line in lines:
    mo = FILE_RE.match(line)
    if mo is not None:
       #print "file #%s: %s" % (mo.group(1), mo.group(2))
       filenames[mo.group(1)] = mo.group(2)
       if mo.group(2)[0] != '<' :
           files[mo.group(1)] = open(mo.group(2)).readlines()
    else :
        mo = LOC_RE.match(line)
        if mo is not None:
            try :
                source = files[mo.group(1)][int(mo.group(2))-1]
            except KeyError :
                source = ''
            line = '{:24}# {:20} {}\n'.format(line[:-1], filenames[mo.group(1)], source)
        else :
            mo = LABELS_RE.match(line)
            if mo is not None :
                line = None
    if line is not None :
        output.append(line)

f = open(filepath,"w")
f.writelines(output)
f.close()
