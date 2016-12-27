d = read.table("tsched.tab", header=TRUE)
h = names(d)
scheds = substr(h[seq(4, length(h), 5)], 0, 3)
nsched = length(scheds)

qs = function (a, b) { return (d[,paste(a, b, sep=".")]) }
q = function (a) { return (d[,a]) }

nrow = 3
ncol = 2
u = 10
pdf("tsched.pdf", height=nrow * u, width=ncol * u)
par(mfrow=c(nrow, ncol))

vp = function (f, ylim, ylab) { 
    i = 1
    plot(c(), xlim=c(0, nsched+1), ylim=ylim, ylab=ylab, xaxt='n')
    for (s in scheds) {
        boxplot(f(s), at=i, names=s, add=TRUE, axes=FALSE, xlab="")
        i = i + 1
    }

    axis(1, at=1:nsched, labels=scheds)
}

vp(function (s) { return (qs(s, "time")) }, ylim=c(0, 10e6), ylab="time")
vp(function (s) { return (q("dem") / q("cap")) }, ylim=c(0, 1), ylab="dem/cap")
vp(function (s) { return (qs(s, "circ") / q("cap")) }, ylim=c(0, 1), ylab="circ/cap")
vp(function (s) { return (qs(s, "circ") / q("dem")) }, ylim=c(0, 1), ylab="circ/dem")
vp(function (s) { return (qs(s, "pend") / q("cap")) }, ylim=c(0, .6), ylab="pend/cap")
vp(function (s) { return (qs(s, "pack") / q("cap")) }, ylim=c(0, .6), ylab="pack/cap")

