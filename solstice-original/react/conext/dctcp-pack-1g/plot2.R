d = read.table("bytes", header=TRUE)
d = d[1:333,]
h = names(d)
nhost = 64
nlane = nhost * nhost

q = function (a) { return (d[,a]) }
n = length(q('nday'))
tunit = .003

pdf("thruput.pdf", height=10, width=12)
par(mfrow=c(2, 1))

xs = seq(n) * tunit

plot(c(), xlim=c(0, n*tunit), ylim=c(0, nhost), 
     xlab="time(s)", ylab="#days")
lines(xs, q('nday'))

plot(c(), xlim=c(0, n*tunit), ylim=c(0, 100), 
     xlab="time(s)", ylab="demand served")
lines(xs, q('dem'), col='gray')
lines(xs, q('pack'), col='red')
lines(xs, q('pack') + q('circ'), col='blue')
abline(h=c(10, 15, 20), col='black', lty=3)
