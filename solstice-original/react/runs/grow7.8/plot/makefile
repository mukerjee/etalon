.PHONY: plot dat ci

plot2:
	./plot2.r

plot:
	./plot.r

dat:
	go build
	./plot

ci:
	cp ./plot2.pdf ~/papers/react/nsdi14/camera-ready/graphs/timeVaryWorkload.pdf
