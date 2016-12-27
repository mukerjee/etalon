d = read.table("dens", header=TRUE)
h = names(d)
nhost = 64
nlane = nhost * nhost

pdf("dens.pdf", height=10, width=5)
par(mfrow=c(2, 1))

q = function (a) { return (d[,a]) }

plot(q('nze'), q('be'), pch=1, cex=.3, xlim=c(0, nlane), ylim=c(0, nlane),
     xlab="Non-zero elments", ylab="Big elements")
plot(q('neph'), q('bph'), pch=1, cex=.3, xlim=c(0, nhost), ylim=c(0, nhost),
     xlab="Non-zero elements per host", ylab="Big elements per host")


pdf("timeline.pdf", height=10, width=12)
par(mfrow=c(2, 1))

n = length(q('nze'))
xs = seq(n) * .003

plot(c(), xlim=c(0, n*.003), ylim=c(0, nlane), xlab="time(s)", ylab="#elements")
lines(xs, q('nze'), col='gray')
lines(xs, q('be'))

print(max(q('be')))

plot(c(), xlim=c(0, n*.003), ylim=c(0, nhost), xlab="time(s)", ylab="#elements/host")
lines(xs, q('neph'), col='gray')
lines(xs, q('nzph90'), col='blue', lty=2)
lines(xs, q('nzph50'), col='red')
lines(xs, q('bph'))


print(max(q('bph')))
