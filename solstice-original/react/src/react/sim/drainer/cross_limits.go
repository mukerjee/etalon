package drainer

import (
	. "react/sim/config"
	. "react/sim/structs"
)

type CrossLimits struct {
	Limits []*Limit

	bandw uint64
	buf   Vector
}

func NewCrossLimits(bandw uint64) *CrossLimits {
	ret := new(CrossLimits)
	ret.bandw = bandw
	ret.Limits = make([]*Limit, 0, Nhost*2)
	ret.Limits = BandwLimits(ret.Limits, bandw)
	ret.buf = NewVector()

	return ret
}

func (self *CrossLimits) LeftOver(used Matrix) {
	buf := self.buf

	used.RowSum(buf)
	buf.Reach(self.bandw)
	for i := 0; i < Nhost; i++ {
		self.Limits[i].Bound = buf[i]
	}

	used.ColSum(buf)
	buf.Reach(self.bandw)
	for i := 0; i < Nhost; i++ {
		self.Limits[Nhost+i].Bound = buf[i]
	}
}
