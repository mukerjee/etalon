d = read.table("bgflows", header=TRUE)
h = names(d)

q = function (a) { return (d[,a]) }
n = length(q('size'))
scaler = 1
if (file.exists('10g')) { scaler = 10 }
if (file.exists('100g')) { scaler = 100 }
print(paste("scaler =", scaler))

pdf("flows.pdf", height=11, width=20)
if (n > 0) {
    par(mfrow=c(1, 3))

    xs = q('size') / 1000 # we have an inherent factor of 10, and an added factor of 100
    ys = q('dura')

    if (FALSE && n > 3e4) {
        inds = seq(n)
        inds = sample(inds, 3e3)
        xs = xs[inds]
        ys = ys[inds]
        n = length(inds)
    }
    plot(xs, ys, xlab='size(bytes)', ylab='duration(us)', cex=0.5, 
         xlim=c(0, 1e6 * scaler), ylim=c(0, 10e4))
    ths = ys / xs
    plot(xs, ths, xlab='size(bytes)', ylab='1/throughput(us/B)', cex=0.5, 
         xlim=c(0, 1e6 * scaler), ylim=c(0, 2e-1 / scaler))
    abline(h=c(0.008, 0.04)/scaler, col='black', lty=3)
    title(basename(getwd()), cex.main=4)
    plot(xs, ys, xlab='size(bytes)', ylab='duration(us)', cex=0.5, 
         xlim=c(0, 7e4), ylim=c(0, 3e4 / scaler))
    # plot(xs, ths * 5, xlab='size(bytes)', ylab='1/throughput(us/B)', cex=0.5, 
    #      xlim=c(0,2e7 * scaler), ylim=c(0, 100e-3 / scaler))
}

d2 = read.table("queries", header=TRUE)
h2 = names(d)
q2 = function (a) { return (d2[,a]) }
xs = q2('start')
ys = q2('dura')
n = length(xs)
inds = seq(n)
yavg = mean(ys)
yavg = quantile(ys, .5, names=FALSE)
y90 = quantile(ys, .9, names=FALSE)
y99 = quantile(ys, .99, names=FALSE)
ymax = max(ys)

if (n > 3e3) {
    inds = sample(inds, 3e3)
    xs = xs[inds]
    ys = ys[inds]
    n = length(inds)
}
# if (n > 1e4) {
#     xs = xs[1:1e4]
#     ys = ys[1:1e4]
# }
print(paste('mid =', yavg))
print(paste('90% =', y90))
print(paste('99% =', y99))
print(ymax)

pdf("queries.pdf", height=15, width=15)
if (n > 0) {
    par(mfrow=c(2,1))
    
    plot(xs, ys, xlab='query start(us)', ylab='duration(us)', cex=0.3,
         # ylim=c(0, 20e4 / scaler) )
         ylim=c(0, ymax))
    title(basename(getwd()), cex.main=2)
    abline(h=yavg, col='black', lty=3)
    abline(h=y90, col='red', lty=3)
    abline(h=y99, col='blue', lty=3)
    plot(xs, ys, xlab='query start(us)', ylab='duration(us)', cex=0.3,
         ylim=c(0, 10e4 / scaler) )
    abline(h=yavg, col='black', lty=3)
    abline(h=y90, col='red', lty=3)
    abline(h=y99, col='blue', lty=3)
}
