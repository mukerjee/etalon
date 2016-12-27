#!/usr/bin/env Rscript

library(RColorBrewer)

dat = read.table('thput')

cols = brewer.pal(8, "Accent")
cols = c(cols[1:3], cols[5:8], 'blue')
ts = dat[,1]
xlim = c(3e6, 50e6)

pdf('plot2.pdf', height=5.5, width=4)
par(mar=c(3, 3.5, 1, 1), mgp=c(1.7, 0.5, 0))
par(mfrow=c(4, 2))
tlabs = c()
for (i in seq(1,6)) { tlabs = c(tlabs, parse(text=paste('t[', i, ']', sep=''))) }

for (i in seq(1,7)) {
    # dat[,1]
    plot(c(), ylim=c(0, 10), xlim=xlim, ylab='Throughput (Gbps)', xlab='Time (ms)', 
        xaxt='n', cex.axis=0.9)
    d = dat[,(8-i)+1]
    rect(dat[,1], d, dat[,1]+1.55e6, 0, col=cols[8-i], border=NA, lwd=0)
    axis(1, at=seq(0,50,by=10)*1e6+3e6, labels=seq(0,50,by=10), cex.axis=0.9)
    abline(v=seq(1,6)*6e6+3e6, lty=2, lwd=0.5)
    text(53e6, 8, paste('Flow', i-1), pos=2, cex=0.9)
    if (i == 1 || i == 2) {
        text(seq(1,6)*6e6+1.2e6, 9.4, tlabs, cex=0.8, pos=4)
    }
}
