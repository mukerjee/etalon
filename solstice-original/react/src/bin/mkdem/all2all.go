package main

import (
	"react/demand"
)

type All2All struct {
	Nhost   int
	Hosts   []int
	LinkBw  uint64
	T       uint64
	Twarmup uint64
}

func (p *All2All) Make() *demand.Demand {
	hosts := p.Hosts
	n := len(hosts)
	if n <= 1 {
		panic("need at least 2 hosts")
	}

	fillBw := p.LinkBw / uint64(n-1)

	ret := demand.NewDemand(p.Nhost)

	if p.Twarmup > 0 {
		ret.Add(demand.NewPeriod(p.Nhost, p.Twarmup))
	}

	period := demand.NewPeriod(p.Nhost, p.T)
	for i := 0; i < n; i++ {
		src := hosts[i]
		for j := 0; j < n; j++ {
			if i == j {
				continue
			}
			dest := hosts[j]

			period.D[src][dest] = fillBw
		}
	}
	ret.Add(period)

	return ret
}
