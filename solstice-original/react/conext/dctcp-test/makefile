.PHONY: plot dat dens bytes

plot:
	Rscript plot.R
	Rscript plot2.R

dat:
	../../bin/dctcp

dens:
	../../bin/dctcp-dens > dens

bytes:
	../../bin/dctcp-sched > bytes
