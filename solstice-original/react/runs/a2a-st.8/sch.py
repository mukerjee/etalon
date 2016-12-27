print "n=8"
print "linkbw=1250"
print "packbw=125"

step=1

for i in range(0, 3000000, 1500):
    print
    print "// T=%d" % i
    for j in range(0, 7):
        stride = (j * step) % 7 + 1
        t = 212
        if j < 4:
            t = 216
        line = "d%d %d:" % (j, t)
        for src in range(0, 8):
            dest = (src + stride) % 8
            assert src != dest
            line += " %d-%d/98" % (src, dest)
        print line
    step = (step + 1) % 7
    if step == 0:
        step = 1
