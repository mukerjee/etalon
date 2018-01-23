#!/usr/bin/python

BASE="10.1"

for i in range(1, 9):
    for j in range(1, 17):
        #if i == 1 and j == 1:
        #    continue
        #addr = BASE+'.'+str(i)+'.'+str(j)+' /rack0'+str(i)
        #addr = '%s.%d.%d h%d%d.sdrt.cs.cmu.edu h%d%d'%(BASE, i,j,i,j, i,j)
        addr = 'h%d%d.sdrt.cs.cmu.edu'%(i,j)
        print addr
