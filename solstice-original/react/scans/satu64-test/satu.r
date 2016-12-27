#!/usr/bin/env Rscript
d = read.table("dat", header=TRUE)
h = names(d)
ids = h[seq(1, length(h), 8)]
ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE) - 1))
n = length(ids)

pdf("satu.pdf", height=3.5, width=10)
par(mfrow=c(1, 3))

doracle = read.table('../satu64oracle/dat', header=TRUE)
doracle2 = read.table('../satu64oracle2/dat', header=TRUE)

vp = function(f, col, ylim, ylab) {
    plot(c(), xlim=c(0, n+1), ylim=ylim, ylab=ylab, xaxt='n', yaxt='n',
         cex.lab=1.2,
         xlab="Number of flows per host")
    i = 1
    for (id in ids) {
        pick = paste("x", id, ".", col, sep="")
        dat = f(d[,pick], id)
        boxplot(dat, at=i, add=TRUE, axes=FALSE, xlab="", 
                range=0, 
                cex=0.2, lwd=0.5,
                boxlwd=0.2,
                whisklty=2, 
                whisklwd=0.4)
        i = i + 1
    }
    # axis(1, at=1:n, labels=ids)
    xids = c(1, seq(5, 50, 5))
    axis(1, at=xids, labels=xids)
}

vl = function(do, col, lty=2) {
    ys = c()
    for (id in ids) {
        pick = paste("x", id, ".", col, sep="")
        dat = do[,pick]
        ys = c(ys, max(dat))
    }
    lines(ids, ys, xlab="", cex=0.9, pch=0, lty=lty)
}

vp(function (d, n) { return (d) }, "nday", ylim=c(0, 50), 
   ylab="Number of configurations")
yids = seq(0, 50, 10)
axis(2, at=yids, labels=yids)
vp(function (d, n) { return (d) }, "ndem", ylim=c(0.75, 1), 
   ylab="Demand served on circuit (%)")
vl(doracle, "ndem")
vl(doracle2, "ndem", lty=3)
yids = seq(0.75, 1, 0.05)
axis(2, at=yids, labels=yids * 100)
legend(1, .80, c("Hybrid Oracle", "Circuit Oracle"), lty=c(3, 2))
# vp(function (d, n) { return (d) }, "ndem2", ylim=c(0.75, 1), 
#    ylab="Fraction of demand served on reactor")
vp(function (d, n) { return (d) }, "ndem3", ylim=c(0.75, 1), 
   ylab="Demand served on hybrid (%)")
vl(doracle, "ndem3")
vl(doracle2, "ndem3", lty=3)
axis(2, at=yids, labels=yids * 100)
legend(1, .80, c("Hybrid Oracle", "Circuit Oracle"), lty=c(3, 2))
