package sim

import (
	. "react/sim/blocks"
	. "react/sim/packet"
)

// entrance is the joint block in the testbed between the hosts
// and the switch, it records the demand, and tells the ongoing
// activities to the demand estimator
type entrance struct {
	send *Counter
	s    Sender
}

var _ Sender = new(entrance)

func newEntrance(s Sender) *entrance {
	ret := new(entrance)
	ret.s = s
	ret.send = NewCounter()
	return ret
}

func (self *entrance) Send(p *Packet) uint64 {
	ret := self.s.Send(p)
	self.send.CountPacket(p)
	return ret
}

func (self *entrance) Clear() {
	self.send.Clear()
}

func (self *entrance) ClearSum() {
	self.send.ClearSum()
}
