package main

import (
	"react/demand"
)

type Grow struct {
	Nhost   int
	Hosts   []int
	Share   int
	Twarmup uint64
	Tstable uint64
	Trest   uint64
	FillBw  uint64
	SmallBw uint64
	Repeat  int
}

func (p *Grow) Make() *demand.Demand {
	ret := demand.NewDemand(p.Nhost)
	hosts := p.Hosts
	n := len(hosts)

	if p.Twarmup > 0 {
		ret.Add(demand.NewPeriod(p.Nhost, p.Twarmup))
	}

	for rep := 0; rep < p.Repeat; rep++ {
		for s := 1; s <= p.Share; s++ {
			nbig := s
			nsmall := p.Share - s
			bigBw := (p.FillBw - p.SmallBw*uint64(nsmall)) / uint64(nbig)
			period := demand.NewPeriod(p.Nhost, p.Tstable)

			for i := 0; i < n; i++ {
				src := hosts[i]
				for j := 0; j < nbig; j++ {
					dest := hosts[(i+1+j)%n]
					period.D[src][dest] = bigBw
				}
				if p.SmallBw == 0 {
					continue
				}
				for j := 0; j < nsmall; j++ {
					dest := hosts[(i+1+nbig+j)%n]
					period.D[src][dest] = p.SmallBw
				}
			}

			ret.Add(period)
		}

		if p.Trest > 0 {
			period := demand.NewPeriod(p.Nhost, p.Trest)
			ret.Add(period)
		}
	}

	return ret
}
