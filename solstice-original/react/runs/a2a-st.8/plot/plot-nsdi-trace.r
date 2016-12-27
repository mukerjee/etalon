#!/usr/bin/env Rscript

library(RColorBrewer)
col = brewer.pal(8, "Accent")

d = read.table('trace', col.names=c('t', 'from', 'type', 'len'))

tmin = 6990
tmax = 4644
d = d[d$t > tmin,]
# d = d[d$t < tmax,]

dat = d[d$type==1,]
weekends = d[d$type==2,]
ts = dat[,'t']
froms = dat[,'from'] + 1
cols = col[froms]

pdf('nsdi-trace.pdf', width=100, height=5)
par(mar=c(0.2,0.2,0.2,0.2))
plot(c(), xlim=c(min(ts), max(ts)), ylim=c(0, 8), bty='n', 
     xaxt='n', yaxt='n', xlab='', ylab='')
rect(ts, froms - 0.4, ts+1.3, froms + 0.4, col=cols, border=cols)
abline(v=weekends[,'t']+10, lty=2, col='black', lwd=2)
abline(h=seq(0, 7) + 0.5, col=gray.colors(20)[10], lty=3, lwd=1)

tbase = min(weekends[,'t'])
text(weekends[,'t'], 0, 
     paste(round(weekends[,'t'] - tbase) / 1e3 + 1.5, 'ms', sep=''), 
     pos=4, cex=2)
