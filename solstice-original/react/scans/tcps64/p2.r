#!/usr/bin/env Rscript

library(RColorBrewer)
library(Hmisc)
cols = brewer.pal(8, "Accent")

d = read.table("dat", header=TRUE)
h = names(d)
ids = h[seq(1, length(h), 8)]
ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE) - 1))
n = length(ids)

pdf("p2.pdf", height=4.5, width=4)

vp = function(f, col, ylim, ylab) {
    plot(c(), xlim=c(0, n+1), ylim=ylim, ylab=ylab, xaxt='n', yaxt='n',
         xlab="Total number of flows", cex.lab=1.2)
    i = 1
    means = c()
    mins = c()
    maxs = c()
    xs = c()
    for (id in ids) {
        pick = paste("x", id, ".", col, sep="")
        dat = f(d[,pick], id)
        means = c(means, mean(dat))
        mins = c(mins, min(dat))
        maxs = c(maxs, max(dat))
        xs = c(xs, i)
        i = i + 1
    }
    # axis(1, at=1:n, labels=ids/10)
    xids = c(20, seq(100, 700, 100))
    axis(1, at=xids/20, labels=xids * 15 / 10)
    errbar(xs, means, mins, maxs, lwd=1, lty=5, cex=0,
           add=TRUE, xlab="", col=cols[3], errbar.col='grey')
    lines(xs, means, cex=1.2, lwd=2, lty=1, col='black')
}

vp(function (d, n) { return (d) }, "ndem", ylim=c(0.75, 1), 
   ylab="Demand served (%)")
yids = seq(0.75, 1, 0.05)
axis(2, at=yids, labels=yids * 100)
