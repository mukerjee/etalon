#!/usr/bin/env Rscript
d = read.table("dat", header=TRUE)
h = names(d)
ids = h[seq(1, length(h), 8)]
ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE) - 1))
n = length(ids)

pdf("satu.pdf", height=9, width=8)
par(mfrow=c(2, 2))

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

vp(function (d, n) { return (d) }, "nday", ylim=c(0, 100), 
   ylab="Number of days")
vp(function (d, n) { return (d) }, "ndem", ylim=c(0.75, 1), 
   ylab="Fraction of demand served on circuit")
vp(function (d, n) { return (d) }, "ndem2", ylim=c(0.75, 1), 
   ylab="Fraction of demand served on reactor")
vp(function (d, n) { return (d) }, "ndem3", ylim=c(0.75, 1), 
   ylab="Fraction of demand served on hybrid")
