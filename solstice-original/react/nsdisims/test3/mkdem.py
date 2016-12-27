nhost=64
t=300000
nbig=8
nsmall=45
psmall=0.5
pall=100.
linkbw=12500

smallbw=0
if nsmall > 0:
    smallbw=int(12500*psmall/100)
bigbw=int((linkbw*pall/100-smallbw*nsmall)/nbig)
print 'nbig=%d nsmall=%d bigbw=%d smallbw=%d' % (
        nbig, nsmall, bigbw, smallbw)

fout = open('demand', 'w')

print >> fout, 'n=%d' % nhost
print >> fout, '% T=0'
print >> fout
print >> fout, 't=%d' % t

line = [bigbw] * nbig + [smallbw] * nsmall + [0] * (nhost - nbig - nsmall)
assert(len(line) == nhost)
for i in range(nhost):
    line = [ line[-1] ] + line[:-1]
    s = ''
    for a in line:
        if a == 0:
            s += ' -'
        else:
            s += (' %d' % a)
    print >> fout, s.strip()

fout.close()
