#!/usr/bin/env Rscript
pdf("nhost.pdf", height=4, width=8)
par(mfrow=c(1, 2))

xids = c(8, 16, 32, 64, 128, 256, 512, 1024)

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
    lines(ids, dat, type="o", xlab="", cex=0.9, pch=pch, lty=3, bg=2)
}

plot(c(), xlim=c(4, 1500), ylim=c(0.8, 1), ylab="Served fraction of the demand", 
     xlab="Number of Nodes", log="x", xaxt='n')
axis(1, at=xids)
p("../nhostlon/dat", pch=0)
p("../nhostmwm/dat", pch=1)
p("../nhostmax/dat", pch=2)
legend(40, 0.98, c("Solstice", "Solstice+MaxSum", "Solstice+MaxMin"), pch=c(0, 1, 2), cex=0.9)

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
    lines(ids, dat / 1e9, type="o", xlab="", cex=0.9, pch=pch, lty=3, bg=2)
}

islip_div = 64
plot(c(), xlim=c(4, 1000), ylim=c(0.0001, 50), ylab="Computation Time (second)", 
     xlab="Number of Nodes", log="xy", xaxt='n')
axis(1, at=xids)
pt("../nhostlon/dat", pch=0)
pt("../nhostmwm/dat", pch=1)
pt("../nhostmax/dat", pch=2)
legend(8, 45, c("Solstice", "Solstice+MaxSum", "Solstice+MaxMin"), pch=c(0, 1, 2), cex=0.9)
