#!/usr/bin/env python

import sys

vars = {}

fp = open(sys.argv[1])
for line in fp:
    if 'define(' in line:
        ls = line.split('define(')[1].split(')')[0].split(', ')
        for l in ls:
            vars[l.split()[0]] = l.split()[1]
        continue
    if '$' in line:
        for v in vars.keys():
            if v in line:
                line = line.replace(v, vars[v])
    sys.stdout.write(line)
