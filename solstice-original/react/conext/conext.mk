.PHONY: plot dat dens bytes flows all

plot:
	Rscript plot.R
	Rscript plot2.R
	Rscript plot3.R

plot4:
	Rscript plot4.R

dat:
	../../bin/dctcp

dens:
	../../bin/dctcp-dens > dens

bytes:
	../../bin/dctcp-sched > bytes

flows:
	../../bin/dctcp-flows

all: dens bytes flows plot
