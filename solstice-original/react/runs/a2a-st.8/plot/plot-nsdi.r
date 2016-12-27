#!/usr/bin/env Rscript

library(RColorBrewer)

thput = read.table('thput', col.names=c('t', paste('h', seq(7), sep='')))
ts = thput[,'t'] / 1e9
nhost = 7
col = brewer.pal(8, "Accent")

pdf('nsdi-thput.pdf', width=8, height=3)
par(mgp=c(2, 1, 0), mar=c(4, 4, 1, 0))
plot(c(), xlim=c(0, 3), ylim=c(0, 10), bty='n', 
     xlab="Time (s)", ylab="TCP Throughput (Gb/s)")
tmax = max(ts)
for (i in seq(nhost)) {
    i = nhost + 1 - i
    d = thput[,paste('h', i, sep='')]
    # lines(ts, d, col='black', lwd=1)
    polygon(c(0, ts, tmax), c(0, d, 0), col=col[i])
    text(3 * (1-0.618), max(d) - 0.6, paste('', i, sep=''))
}
