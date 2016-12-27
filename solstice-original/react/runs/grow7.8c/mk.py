
week = 1500
smallbw = 12
totalbw = 1050
nhost = 8
night = 30
linkbw = 1250
packbw = 125
circbw = linkbw - packbw

def tsplit(nday):
    day = week / nday
    ret = [day] * nday
    residue = week % nday
    for i in range(residue):
        ret[i] += 1
    assert(sum(ret) == week)
    return ret

def dsplit(nbig):
    nsmall = nhost - 1 - nbig
    bigbw = (totalbw - smallbw * nsmall) / nbig
    startbw = linkbw
    # startbw = startbw * 9 / 10
    dur = [bigbw] * nbig + [smallbw] * nsmall
    start = dur
    if nbig > 1:
        start = [bigbw] * (nbig - 1) + [startbw] + [smallbw] * nsmall
    return dur, start

def mkdem(pdem='demand', psch='sim.sch'):
    fdem = open(pdem, 'w')
    print >> fdem, 'n=%d' % nhost
    demt = [0]

    fsch = open(psch, 'w')
    print >> fsch, 'n=%d' % nhost
    print >> fsch, 'linkbw=%d' % linkbw
    print >> fsch, 'packbw=%d' % packbw
    scht = [0]

    def emit_dem(t, d):
        print >> fdem
        print >> fdem, '// T=%d' % demt[0]
        demt[0] += t
        print >> fdem, 't=%d' % t
        line = [0] + d
        for i in range(nhost):
            s = ''
            for a in line:
                if a == 0:
                    s += ' -'
                else:
                    s += ' %d' % a
            print >> fdem, s.strip()
            line = [line[-1]] + line[:-1]
    
    def emit_sch(ts=[], bw=0):
        print >> fsch
        print >> fsch, '// T=%d' % scht[0]
        scht[0] += week
        if not ts:
            print >> fsch, 'd0 %d:' % week
            return
        
        nday = len(ts)
        for i in range(nday):
            t = ts[i]
            s = ''
            s += 'd%d %d:' % (i, t)
            tday = t - night
            b = bw * week / tday
            if bw * week % tday != 0:
                b += 1
            if b < circbw:
                b = circbw
            b = 100 * b / linkbw
            if b > 99:
                b = 99
            for src in range(nhost):
                dest = (src + nday - i) % nhost
                s += ' %d-%d/%d' % (src, dest, b)
            print >> fsch, s.strip()

    def emit_schs(rep, ts=[], bw=0):
        for i in range(rep):
            emit_sch(ts, bw)
    
    emit_dem(week * 2, [0] * (nhost - 1))
    emit_schs(2)

    for i in range(1, nhost):
        ts = tsplit(i)
        ds, ds_ = dsplit(i)
        emit_dem(ts[0], ds_)
        emit_dem(week * 4 - ts[0], ds)
        # emit_dem(week * 4, ds)
        emit_schs(4, ts, ds[0])
    
    fsch.close()
    fdem.close()

mkdem()
