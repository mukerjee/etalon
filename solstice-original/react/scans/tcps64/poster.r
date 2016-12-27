#!/usr/bin/env Rscript

d = read.table("dat", header=TRUE)
h = names(d)
ids = h[seq(1, length(h), 8)]
ids = strtoi(substr(ids, 2, regexpr(".", ids, fixed=TRUE) - 1))
n = length(ids)

pdf("poster.pdf", height=4, width=4)

col = "ndem"
ylab = "Demand Served"
ylim = c(.5, 1)
plot(c(), xlim=c(min(ids), max(ids)), ylim=ylim, ylab=ylab, xaxt='n', yaxt='n',
     xlab="Total Number of Flows")
i = 1
mids = c()
for (id in ids) {
    pick = paste("x", id, ".", col, sep="")
    dat = d[,pick]
    # boxplot(dat, at=i, add=TRUE, axes=FALSE, xlab="", 
    #         range=0, cex=0.5, lwd=0.5, boxlwd=0.2)
    mids = c(mids, mean(dat))
    i = i + 1
}

lines(ids, mids, type="o", xlab="", cex=.5, pch=0, lty=2, bg=1)
axis(1, at=ids, ids)
axis(2, (0:10)/10, labels=paste((0:10)*10, '%', sep=''))
