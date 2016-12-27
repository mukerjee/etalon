package main

import (
	"react/demand"
)

type PatSwitch struct {
	Nhost  int
	Hosts  []int
	LinkBw uint64

	Twarmup uint64
	Tsep    uint64
	Tcross  uint64
	Repeat  int
}

func (p *PatSwitch) Make() *demand.Demand {
	hosts := p.Hosts
	n := len(hosts)
	if n < 4 {
		panic("need at least 4 hosts")
	}
	half := n / 2

	ret := demand.NewDemand(p.Nhost)

	if p.Twarmup > 0 {
		ret.Add(demand.NewPeriod(p.Nhost, p.Twarmup))
	}

	for rep := 0; rep < p.Repeat; rep++ {
		period := demand.NewPeriod(p.Nhost, p.Tsep)
		for i := 0; i < n; i++ {
			src := hosts[i]
			for j := 0; j < n; j++ {
				if i == j {
					continue
				}
				dest := hosts[j]

				if i < half && j < half {
					period.D[src][dest] = p.LinkBw / uint64(half-1)
				} else if i >= half && j >= half {
					period.D[src][dest] = p.LinkBw / uint64(n-half-1)
				}
			}
		}
		ret.Add(period)

		period = demand.NewPeriod(p.Nhost, p.Tcross)
		for i := 0; i < n; i++ {
			src := hosts[i]
			for j := 0; j < n; j++ {
				if i == j {
					continue
				}
				dest := hosts[j]

				if i < half && j >= half {
					period.D[src][dest] = p.LinkBw / uint64(n-half)
				} else if i >= half && j < half {
					period.D[src][dest] = p.LinkBw / uint64(half)
				}
			}
		}
		ret.Add(period)
	}

	return ret
}
