package bvn

import (
	"react/sim"
	. "react/sim/config"
	. "react/sim/structs"
)

type HotSpotScheduler struct {
	demand Matrix
	slicer Slicer
	bandw  Matrix
	pm     PortMap
	day    *Day
}

var _ sim.Scheduler = new(HotSpotScheduler)

func NewHotSpotScheduler() *HotSpotScheduler {
	ret := new(HotSpotScheduler)
	slicer := NewMaxSumSlicer()
	ret.slicer = slicer
	slicer.AcceptShorter = true

	ret.demand = NewMatrix()
	ret.bandw = NewMatrixOf(LinkBw - PackBw)
	ret.pm = NewPortMap()

	ret.slicer.Reset()

	return ret
}

func (self *HotSpotScheduler) Schedule(r, d Matrix,
	_ uint64) ([]*Day, Matrix) {
	if r != nil {
		copy(self.demand, r)
	} else {
		self.demand.Clear()
	}

	self.demand.Madd(d)
	sp := self.demand.Sparse()
	self.pm.Clear()
	self.slicer.Slice(sp, self.pm)
	day := NewDay(self.pm.Matrix(), WeekLen)
	self.bandw.Clear()
	self.bandw.AddInMap(self.pm, LinkBw-PackBw)

	return []*Day{day}, self.bandw
}
