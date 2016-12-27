package tors

import (
	. "react/sim"
	. "react/sim/blocks"
	. "react/sim/packet"
	. "react/sim/queues"
	. "react/sim/structs"
)

type Fabrics struct {
	buf    *Queues
	Demand Matrix
}

var _ Switch = new(Fabrics)

func NewFabrics() *Fabrics {
	ret := new(Fabrics)
	ret.buf = NewQueues()
	ret.Demand = NewMatrix()
	return ret
}

func (self *Fabrics) Send(packet *Packet) uint64 {
	return self.buf.Send(packet)
}

func (self *Fabrics) Tick(sink Block, _ Estimator) (Matrix, Events) {
	self.buf.MoveAll(sink, Destination, self.Demand)
	return nil, 0
}

func (self *Fabrics) Tdma() bool                         { return false }
func (self *Fabrics) Bind(_ Scheduler, _ *SchedRecorder) {}
func (self *Fabrics) Served() (circ Matrix, pack Matrix) {
	return nil, self.Demand
}
