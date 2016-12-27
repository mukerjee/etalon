#!/usr/bin/Rscript

d1 = read.table("align.lats", header=FALSE)
ts1 = d1[,1]
lats1 = d1[,2]
pdf("plot.pdf", heigh=5, width=5)
# plot(ts, lats)
res1 = ecdf(lats1)
plot(c(), xlab="Latency (microsecond)", xlim=c(0, 5500), main="", ylab="", ylim=c(0, 1))
# lines(res1, do.points=FALSE, lty=1, verticals=TRUE, lwd=1, col="red")
with(environment(res1), lines(c(0, x), c(0, y), col="red", lwd=1.5, lty=2))
# lines(as.list(environment(res1)), col="red", lwd=1, lty=1)
title("CDF of control path latencies")

d2 = read.table("sols.lats", header=FALSE)
ts2 = d2[,1]
lats2 = d2[,2]

res2 = ecdf(lats2)
# lines(res2, do.points=FALSE, lty=2, verticals=TRUE, lwd=1 , col="blue")
# lines(as.list(environment(res2)), col="blue", lwd=1, lty=2)
with(environment(res2), lines(c(0, x), c(0, y), col="blue", lwd=1.5, lty=1))
abline(h=0, col="gray", lwd=.7, lty=2)
abline(h=1, col="gray", lwd=.7, lty=2)
legend(1500, 0.3, c("Solstice, serving 92%", "Align, serving 87%"), 
    col=c("blue", "red"), lty=c(1, 2), lwd=1.5)

print(quantile(lats1, c(.5, .9, .99, 1)))
print(quantile(lats2, c(.5, .9, .99, 1)))
