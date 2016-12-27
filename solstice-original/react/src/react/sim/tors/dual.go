package tors

import (
	. "react/sim"
	. "react/sim/blocks"
	"react/sim/clock"
	. "react/sim/config"
	. "react/sim/drainer"
	. "react/sim/queues"
	. "react/sim/structs"
)

type DualSwitch struct {
	nics        *Queues
	bigSwitch   *Queues
	smallSwitch *Queues
	downRaters  *Queues

	NicQueue  Matrix
	BigLink   Matrix
	SmallLink Matrix

	BigUp      Matrix
	BigBuf     Matrix
	BigBufDrop Matrix
	BigDown    Matrix

	SmallUp      Matrix
	SmallBuf     Matrix
	SmallBufDrop Matrix
	SmallDown    Matrix

	DownThru Matrix
	DownDrop Matrix

	Demand Matrix

	smallLimits []*Limit
	rowBuf      Vector

	bigUpDrainer     *LinkDrainer
	smallUpDrainer   *Drainer
	smallDownDrainer *LinkDrainer
	downDrainer      *LinkDrainer
}

func NewDualSwitch() *DualSwitch {
	ret := new(DualSwitch)
	ret.nics = NewNics()

	bufSize := SwitchBufSize()
	ret.bigSwitch = NewSizedQueues(bufSize)
	ret.smallSwitch = NewSizedQueues(bufSize)
	ret.downRaters = NewQueues()

	ret.rowBuf = NewVector()

	limits := make([]*Limit, 0, Nhost+1)
	limits = RowLimits(limits, PackBw)
	limits = append(limits, NewLimit(make([]int, 0, Nlane), 0))
	ret.smallLimits = limits
	ret.downDrainer = NewDownlinkDrainer(LinkBw)
	ret.bigUpDrainer = NewUplinkDrainer(LinkBw)

	ret.Demand = NewMatrix()

	return ret
}

func (s *DualSwitch) Tick(sink Block, e Estimator) (Matrix, Events) {
	if clock.T%WeekLen == 0 {
		estimate, n := e.Estimate()
		copy(s.Demand, estimate)
		threshold := n * LinkBw * 6 / 10
		s.Demand.TrimUnder(threshold)
		s.Demand.Binary()
		s.Demand.Mul(LinkBw)
	}

	panic("todo")
}
