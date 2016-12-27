.PHONY: plot all ci

all:
	go build
	./plot
	./plot.r

plot:
	./plot.r

ci:
	cp plot.pdf ~/papers/react/nsdi14/camera-ready/figs/eval-e4.pdf
