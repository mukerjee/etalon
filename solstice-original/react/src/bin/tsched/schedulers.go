package main

import (
	"encoding/json"
	"react/sim/bvn"
)

type Schedulers struct {
	Aligns   []bool
	Stuffers []string
	Slicers  []string
}

func (self *Schedulers) Make() []*bvn.Scheduler {
	aligns := self.Aligns
	stuffers := self.Stuffers
	slicers := self.Slicers

	if len(aligns) == 0 {
		aligns = []bool{false, true}
	}
	if len(stuffers) == 0 {
		stuffers = []string{"quick", "skewed", "flat"}
	}
	if len(slicers) == 0 {
		slicers = []string{"any", "max-sum", "max", "islip", "laura"}
	}

	n := len(aligns) * len(stuffers) * len(slicers)
	ret := make([]*bvn.Scheduler, 0, n)

	for _, align := range aligns {
		for _, stuffer := range stuffers {
			for _, slicer := range slicers {
				s := &Scheduler{align, stuffer, slicer}
				ret = append(ret, s.Make())
			}
		}
	}
	return ret
}

func ParseSchedulers(dec *json.Decoder) (*Schedulers, error) {
	ret := new(Schedulers)
	e := dec.Decode(ret)
	return ret, e
}
