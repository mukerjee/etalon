d = read.table("dat", header=TRUE)
h = names(d)
ids = h[seq(1, length(h), 8)]
ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE) - 1))
n = length(ids)

pdf("plot.pdf", height=40, width=20)
par(mfrow=c(4, 2))

vp = function(f, col, ylim, ylab) {
    plot(c(), xlim=c(0, n+1), ylim=ylim, ylab=ylab, xaxt='n')
    i = 1
    for (id in ids) {
        pick = paste("x", id, ".", col, sep="")
        dat = f(d[,pick], id)
        boxplot(dat, at=i, add=TRUE, axes=FALSE, xlab="")
        i = i + 1
    }
    axis(1, at=1:n, labels=ids)
}

vp(function (d, n) { return (d) }, "t", ylim=c(0, 30e6), ylab="time used")
vp(function (d, n) { return (d) }, "tday", ylim=c(0, 30e6), ylab="time used per day")
vp(function (d, n) { return (d) }, "nedge", ylim=c(0, 300), ylab="#edges looked")
vp(function (d, n) { return (d) }, "nday", ylim=c(0, 100), ylab="#days")
vp(function (d, n) { return (d) }, "nbyte", ylim=c(.6, 1), ylab="#nbytes")
vp(function (d, n) { return (d) }, "ndem", ylim=c(.6, 1), ylab="#nbytes")
vp(function (d, n) { return (d) }, "ndem2", ylim=c(.6, 1), ylab="#nbytes")
vp(function (d, n) { return (d) }, "ndem3", ylim=c(.6, 1), ylab="#nbytes")
