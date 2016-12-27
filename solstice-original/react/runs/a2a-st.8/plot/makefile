.PHONY: nsdi plot all ci prog

nsdi:
	./plot-nsdi-trace.r
	./plot-nsdi.r

plot:
	./plot.r

prog:
	go build

dat:
	./plot

ci:
	cp ./e2.pdf ~/papers/react/nsdi14/camera-ready/figs/eval-e2.pdf
