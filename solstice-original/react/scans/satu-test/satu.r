#!/usr/bin/env Rscript
d = read.table("dat", header=TRUE)
h = names(d)
ids = h[seq(1, length(h), 6)]
ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE) - 1))
n = length(ids)

pdf("satu.pdf", height=4.5, width=8)
par(mfrow=c(1, 2))

vp = function(f, col, ylim, ylab) {
    plot(c(), xlim=c(0, n+1), ylim=ylim, ylab=ylab, xaxt='n', 
         xlab="Number of flows per host")
    i = 1
    for (id in ids) {
        pick = paste("x", id, ".", col, sep="")
        dat = f(d[,pick], id)
        boxplot(dat, at=i, add=TRUE, axes=FALSE, xlab="", cex=0.2)
        i = i + 1
    }
    axis(1, at=1:n, labels=ids)
}

vp(function (d, n) { return (d) }, "nday", ylim=c(0, 50), 
   ylab="Number of days")
vp(function (d, n) { return (d) }, "ndem", ylim=c(0.75, 1), 
   ylab="Fraction of demand served")
