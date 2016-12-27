.PHONY: plot dat ci

plot:
	./plot.r

dat:
	go build
	./plot

ci:
	cp ./plot.pdf ~/papers/react/nsdi14/camera-ready/graphs/timeVaryWorkload.pdf
