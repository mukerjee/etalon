d = read.table("bgflows", header=TRUE)
h = names(d)
q = function (a) { return (d[,a]) }
n = length(q('size'))
scaler = 1
if (file.exists('10g')) { scaler = 10 }
if (file.exists('100g')) { scaler = 100 }
print(paste("scaler =", scaler))

xs = q('size') / 1000
ys = q('dura')

pdf("simple.pdf", height=4, width=12)
par(mfrow=c(1, 3))
bigs = ys[xs >= 5000]
smalls = ys[xs < 5000]
bigsize = mean(xs[xs >= 5000])
smallsize = mean(xs[xs < 5000])
plot(c(), c(), ylab='duration(us)', xlim=c(0, 3), ylim=c(0, 5e4))
boxplot(smalls, at=1, add=TRUE, axes=FALSE, xlab='', 
        range=0, cex=1, lwd=2, boxlwd=1, whisklty=1, whisklwd=1)
boxplot(bigs, at=2, add=TRUE, axes=FALSE, xlab='',
        range=0, cex=1, lwd=2, boxlwd=1, whisklty=1, whisklwd=1)

smallscdf = ecdf(smalls)
plot(smallscdf)

bigscdf = ecdf(bigs)
plot(bigscdf)
