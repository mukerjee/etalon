plot:
	Rscript ../plot.r

.PHONY: dat ci
dat:
	../../bin/scansch

ci:
	@ ./ci
