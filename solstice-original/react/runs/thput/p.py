f = open('thput')

lines = f.readlines()
nlines = len(lines)
assert(nlines % 6 == 0)
n = nlines / 6

cap = 1250 * 8 * 1500
pe = lambda a: a/cap*100
fact = (1402. + 24) / 1400

print 'b.s   %5s   %5s   %5s  -  %5s   %5s   %5s' % (
        'sim', 'run', 'diff', 'simc', 'runc', 'diff'
        )

for i in range(n):
    tag = lines[i*6].strip()
    nbig = int(tag[5])
    nsmall = int(tag[6])
    assert(tag.startswith('.'))
    sim = float(lines[i*6+1].strip())
    run = float(lines[i*6+2].strip()) * fact
    simp = float(lines[i*6+4].strip())
    runp = float(lines[i*6+5].strip()) * fact

    print '%d.%d   %5.2f   %5.2f   %5.2f  -  %5.2f   %5.2f   %5.2f' % (
            nbig, nsmall, 
            pe(sim), pe(run), pe(sim-run),
            pe(simp), pe(runp), pe(simp-runp),
            )

f.close()
