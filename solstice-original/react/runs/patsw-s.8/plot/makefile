.PHONY: nsdi plot all ci

nsdi:
	./plot-nsdi.r

plot:
	./plot.r

all:
	go build

ci:
	cp plot.pdf ~/papers/react/nsdi14/camera-ready/figs/eval-e4.pdf
