#!/usr/bin/python

output = open('topology.data','w')

for i in range(1, 4):
    for j in range (1, 9):
        output.write('h%d%d\t/rack0%d\n'%(i,j,i))
        output.write('10.10.1.%d%d\t/rack0%d\n'%(i,j,i))
output.close()


