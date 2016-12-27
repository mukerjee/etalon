#!/usr/bin/env Rscript

library(RColorBrewer)
library(Hmisc)
cols = brewer.pal(8, "Accent")

d = read.table("dat", header=TRUE)
h = names(d)
ids = h[seq(1, length(h), 8)]
ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE) - 1))
n = length(ids)


doracle = read.table('../satu64oracle/dat', header=TRUE)
doracle2 = read.table('../satu64oracle2/dat', header=TRUE)

cp = function(f, col, ylim, ylab, skipbox=FALSE) {
    plot(c(), xlim=c(0, n+1), ylim=ylim, ylab=ylab, xaxt='n', yaxt='n',
         cex.lab=1.2,
         xlab="Number of flows per host")
}

vp = function(f, col, ylim, ylab, skipbox=FALSE) {
    i = 1
    means = c()
    mins = c()
    maxs = c()
    for (id in ids) {
        pick = paste("x", id, ".", col, sep="")
        dat = f(d[,pick], id)
        if (!skipbox) {
            if (FALSE) {
                boxplot(dat, at=i, add=TRUE, axes=FALSE, xlab="", 
                        range=0, 
                        cex=0.2, lwd=0.5,
                        boxlwd=0.2,
                        varwidth=0.2,
                        whisklty=2, 
                        whisklwd=0.4)
            }
            means = c(means, mean(dat))
            mins = c(mins, min(dat))
            maxs = c(maxs, max(dat))
        }
        i = i + 1
    }
    # axis(1, at=1:n, labels=ids)
    xids = c(1, seq(5, 50, 5))
    axis(1, at=xids, labels=xids)
    if (!skipbox) {
        errbar(ids, means, mins, maxs, lwd=1, lty=5, cex=0,
               add=TRUE, xlab="",
               col=cols[3],
               errbar.col='grey')
        lines(ids, means, cex=1.2, lwd=2, lty=1, col='black')
    }
}

vl = function(do, col, lty=2, c='black') {
    ys = c()
    for (id in ids) {
        pick = paste("x", id, ".", col, sep="")
        dat = do[,pick]
        ys = c(ys, max(dat))
    }
    lines(ids, ys, xlab="", cex=1.2, lwd=2, pch=0, lty=lty, col=c)
}

pdf("poster.pdf", height=4.5, width=4)
cp(function (d, n) { return (d) }, "ndem", ylim=c(0.75, 1), 
   ylab="Demand served (%)", skipbox=TRUE)
vl(doracle, "ndem", c=cols[2])
vl(doracle2, "ndem", lty=4, c=cols[1])
vp(function (d, n) { return (d) }, "ndem", ylim=c(0.75, 1), 
   ylab="Demand served (%)", skipbox=TRUE)
yids = seq(0.75, 1, 0.05)
axis(2, at=yids, labels=yids * 100)
legend(1, .81, c("Hybrid Ref", "Circuit Ref"), lwd=2, lty=c(4, 2), col=c(cols[1], cols[2]))

pdf("poster2.pdf", height=4.5, width=4)
cp(function (d, n) { return (d) }, "ndem", ylim=c(0.75, 1), 
   ylab="Demand served (%)")
vl(doracle, "ndem", c=cols[2])
vl(doracle2, "ndem", lty=4, c=cols[1])
vp(function (d, n) { return (d) }, "ndem", ylim=c(0.75, 1), 
   ylab="Demand served (%)", skipbox=FALSE)
yids = seq(0.75, 1, 0.05)
axis(2, at=yids, labels=yids * 100)
legend(1, .83, c("Hybrid Ref", "Circuit Ref", "Solstice"), lwd=2, lty=c(4, 2, 1), 
       col=c(cols[1], cols[2], 'black'))
