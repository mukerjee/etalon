#!/usr/bin/env Rscript
pdf("moti.pdf", height=10, width=4)
par(mfrow=c(3, 1))

xs = seq(0, 30)
ys = c()
for (x in xs) {
    if (x == 0) {
        ys = c(ys, 64)
    } else {
        y = floor(3000 / x / 20)
        if (y > 64) {
            y = 64
        }
        ys = c(ys, y)
    }
}

xids = seq(0, 1, 0.2)
xlabs = paste(xids, '%', sep='')

plot(xs / 30, ys, xlim=c(0, 1), ylim=c(0, 64), ylab="Number of flows per port", 
     xlab=expression(paste(delta, " / W")), cex.lab=1.2, xaxt='n')
axis(1, at=xids, labels=xlabs)

p = function(fname, pch=0) {
    d = read.table(fname, header=TRUE)
    h = names(d)
    ids = h[seq(1, length(h), 8)]
    ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE)-1))
    n = length(ids)
    dat = c()
    for (id in ids) {
        pick = paste("x", id, ".", "ndem", sep="")
        dat = c(dat, mean(d[,pick]))
    }
    lines(ids / 30, dat, type="o", xlab="", cex=0.9, pch=pch, lty=3, bg=2)
}

plot(c(), xlim=c(0, 1), ylim=c(0.4, 1), ylab="Served fraction of the demand", 
     xlab=expression(paste(delta, " / W")), cex.lab=1.2, xaxt='n')
axis(1, at=xids, labels=xlabs)
p("../n64lon-c/dat", pch=0)
p("../n64bvn/dat", pch=1)
p("../n64islip4x/dat", pch=2)
p("../n64islipx/dat", pch=3)
p("../n64alignx/dat", pch=4)
legend(24/30, 0.6, c("Solstice", "Aligning", "BvN", "iSLIP-4", "iSLIP"), pch=c(0, 4, 1, 2, 3), cex=0.9)

pt = function(fname, pch=0, div=1) {
    d = read.table(fname, header=TRUE)
    h = names(d)
    ids = h[seq(1, length(h), 8)]
    ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE)-1))
    n = length(ids)
    dat = c()
    for (id in ids) {
        pick = paste("x", id, ".", "t", sep="")
        dat = c(dat, mean(d[,pick]) / div)
    }
    lines(ids / 30, dat, type="o", xlab="", cex=0.9, pch=pch, lty=3, bg=2)
}

islip_div = 64
plot(c(), xlim=c(0, 1), ylim=c(0, 60e6), ylab="Computation time (ms)", 
     xlab=expression(paste(delta, " / W")), yaxt='n', cex.lab=1.2, xaxt='n')
axis(1, at=xids, labels=xlabs)
yids = seq(0, 60, 10)
axis(2, at=yids * 1e6, yids)
pt("../n64lon/dat", pch=0)
pt("../n64bvn/dat", pch=1)
pt("../n64islip4x/dat", pch=2)
pt("../n64islipx/dat", pch=3)
pt("../n64alignx/dat", pch=4)
pt("../n64islip4x/dat", pch=5, div=islip_div)
pt("../n64islipx/dat", pch=6, div=islip_div)
legend(20/30, 50e6, c("BvN", "iSLIP-4", "iSLIP", "Solstice", "Aligning", "iSLIP-4-Parallel", "iSLIP-Parallel"), 
       pch=c(1, 2, 3, 0, 4, 5, 6), cex=0.9)
abline(h=3e6, lty=4)
