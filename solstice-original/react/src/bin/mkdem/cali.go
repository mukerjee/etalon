package main

import (
	"react/demand"
)

type Calibrate struct {
	Nhost   int
	Nbig    int
	Nsmall  int
	Hosts   []int
	FillBw  uint64
	SmallBw uint64
	Twarmup uint64
	Tstable uint64
	Trest   uint64
	Tlead   uint64
	Tclean  uint64
}

func (p *Calibrate) Make() *demand.Demand {
	ret := demand.NewDemand(p.Nhost)
	hosts := p.Hosts
	n := len(hosts)

	if p.Twarmup > 0 {
		ret.Add(demand.NewPeriod(p.Nhost, p.Twarmup))
	}

	// for nbig := 1; nbig < n; nbig++ {
	// for nsmall := 0; nsmall < n-nbig; nsmall++ {
	nbig := p.Nbig
	nsmall := p.Nsmall
	bigBw := (p.FillBw - p.SmallBw*uint64(nsmall)) / uint64(nbig)
	smallBw := p.SmallBw
	period := demand.NewPeriod(p.Nhost, p.Tstable)

	for i := 0; i < n; i++ {
		src := hosts[i]

		for j := 0; j < nbig; j++ {
			dest := hosts[(i+j+1)%n]
			period.D[src][dest] = bigBw
		}

		for j := nbig; j < nbig+nsmall; j++ {
			dest := hosts[(i+j+1)%n]
			period.D[src][dest] = smallBw
		}
	}
	ret.Add(period)

	if p.Trest > 0 {
		ret.Add(demand.NewPeriod(p.Nhost, p.Trest))
	}

	if p.Tclean > 0 {
		period := demand.NewPeriod(p.Nhost, p.Tclean)
		for i := 0; i < n; i++ {
			src := hosts[i]
			for j := 0; j < n-1; j++ {
				dest := hosts[(i+j+1)%n]
				period.D[src][dest] = 1
			}
		}
		ret.Add(period)
	}

	if p.Tlead > 0 {
		ret.Add(demand.NewPeriod(p.Nhost, p.Tlead))
	}
	//	}
	// }

	return ret
}
