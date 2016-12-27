#!/usr/bin/env Rscript

d = read.table("dat", header=TRUE)
h = names(d)
n = length(h)

pdf("scomp.pdf", height=3.5, width=10)
par(mfrow=c(1, 3))

vp = function(f, ylim, ylab) {
    plot(c(), xlim=c(0, n+1), ylim=ylim, ylab=ylab, xaxt='n', 
         xlab="Number of ports", cex.lab=1.2)
    i = 1
    for (nhost in h) {
        nh = strtoi(substring(nhost, 2))
        dat = f(d[,nhost], nh)
        boxplot(dat, at=i, add=TRUE, axes=FALSE, xlab="",
                range=0, lwd=0.7)
        i = i + 1
    }
    header = substr(h, 2, 1000)
    # axis(1, at=1:n, labels=header)
    axis(1, at=1:n, labels=c(8, 16, 32, 64, "", 256, "", 1024))
}

vp(function (d, n) { return (d / n) }, ylim=c(0, 50), 
   ylab="Edges searched over N")
vp(function (d, n) { return (d / n / log(n)) }, ylim=c(0, 10), 
   ylab="Edges searched over (N log N)")
vp(function (d, n) { return (d / n / log(n) / log(n)) }, ylim=c(0, 3), 
   ylab="Edges searched over (N log^2(N))")
