package pattern

import (
	dem "react/demand"
	. "react/sim/config"
	. "react/sim/structs"
)

type Timeline []*Tick

func NewTimeline(n uint64) Timeline {
	ret := make([]*Tick, n)
	for i := uint64(0); i < n; i++ {
		ret[i] = NewTick()
	}

	return Timeline(ret)
}

func emit(d *dem.Demand, m Matrix, n uint64) {
	if n == 0 {
		panic("bug")
	}

	p := dem.NewPeriod(Nhost, n)
	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			if i == j {
				continue
			}
			lane := i*Nhost + j
			p.D[i][j] = m[lane]
		}
	}

	d.Add(p)
}

func (self Timeline) AppendTo(d *dem.Demand) {
	var last Matrix
	n := uint64(0)
	for _, t := range self {
		this := t.D
		if last == nil {
			last = this
			n = 1
		} else if last.Equals(this) {
			n++
		} else {
			emit(d, last, n)

			last = this
			n = 1
		}
	}

	if last != nil {
		emit(d, last, n)
	}
}
