#!/usr/bin/env Rscript

library(RColorBrewer)

dat = read.table('trace')
week_end = c()
sched_updates = c()
pfcs = c()
pack_ts = c()
pack_lens = c()
pack_froms = c()
pack_cols = c()
cols = brewer.pal(8, "Accent")
cols = c(cols[1:3], cols[5:8], 'blue')
nrow = dim(dat)[1]

for (i in seq(1, nrow)) {
    typ = dat[i,3]
    src = dat[i,2]
    t = dat[i,1]
    sz = dat[i,4]
    
    if (typ == 4) { # pfc
        pfcs = c(pfcs, t)
    } else if (typ == 2) { # weekend
        week_end = c(week_end, t)
    } else if (typ == 5) {
        sched_updates = c(sched_updates, t)
    } else if (typ == 0) {
        pack_froms = c(pack_froms, src)
        pack_ts = c(pack_ts, t)
        pack_lens = c(pack_lens, sz)
        pack_cols = c(pack_cols, cols[src+1])
    }
}
tmin = min(dat[,1])
tmax = max(dat[,1])

pdf('plot.pdf', height=3.5, width=6)
par(mar=c(2.4, 1, 1, 1))
par(mgp=c(1, 0, 0))
plot(c(), xaxt='n', type='n', bty='n', xlab='Time (us)', yaxt='n',
    ylab='', cex.lab=1.0, cex.axis=0.7, xlim=c(tmin - 1300, tmax), ylim=c(0,9))

abline(h=seq(0,9)+0.5, lwd=1, lty=2, col=gray.colors(20)[15])
abline(v=week_end+30, lty=2, col='red')
rect(pack_ts, pack_froms+0.2, pack_ts+6, pack_froms+0.8,
     col=pack_cols, border=NA)

# abline(v=sched_updates, lty=3, col='black')
rect(pfcs, 7.2, pfcs+10, 7.8, col='black', border=NA)
rect(sched_updates, 8.2, sched_updates+10, 8.8, col='red', border=NA)

# text_cex=0.8
ylabs=c(paste('host', 6-seq(0,5), sep=''), 'Rx from host0', 'PFC', 'Reconfig')
text(0, seq(0,9)+0.7, ylabs, cex=0.9, pos=2)
# axis(2, at=seq(0,9), labels=ylabs, lwd=0, cex.axis=0.8, srt=45, pos=1)
# text(0, seq(0,6)+0.7, paste(seq(0,6), ' -> 7', sep=''), cex=text_cex, pos=2)
# text(0, 7.7, 'PFC', cex=text_cex, pos=2)
# text(0, 8.7, 'Reconfig', cex=text_cex, pos=2)

axis(1, at=week_end+30, labels=round((week_end - week_end[1]) / 1500) * 1500, lwd=0, cex.axis=0.9)

