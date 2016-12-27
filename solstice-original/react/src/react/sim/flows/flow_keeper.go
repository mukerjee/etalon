package flows

import (
	"sort"

	"react/sim"
	. "react/sim/blocks"
	"react/sim/clock"
	. "react/sim/config"
	. "react/sim/drainer"
	. "react/sim/packet"
	. "react/sim/structs"
)

var FlowBandwFactor float64 = 0.95

// A flow keeper generates traffic based existing flows.
// A flow keeper never over-subscribes the link.
type FlowKeeper struct {
	flows map[uint64]*Flow

	FlowCount  Matrix
	TargetRate Matrix
	flowSeq    uint64

	limits []*Limit
	budget Matrix
	driver *FlowDriver

	drainer *Drainer
}

var _ sim.Hosts = new(FlowKeeper)

func NewFlowKeeper() *FlowKeeper {
	ret := new(FlowKeeper)
	ret.flows = make(map[uint64]*Flow)
	ret.FlowCount = NewMatrix()
	ret.TargetRate = NewMatrix()
	ret.flowSeq = 1

	flowBw := uint64(float64(LinkBw-uint64(Nhost)) * FlowBandwFactor)
	ret.drainer = NewBandwDrainer(flowBw)
	ret.budget = NewMatrixOf(flowBw)

	return ret
}

type Flows []*Flow

func (s Flows) Len() int      { return len(s) }
func (s Flows) Swap(i, j int) { s[i], s[j] = s[j], s[i] }
func (s Flows) Less(i, j int) bool {
	if s[i].size == s[j].size {
		return s[i].id < s[j].id
	}
	return s[i].size > s[j].size
}

func (self *FlowKeeper) Nflow() int { return len(self.flows) }

func (self *FlowKeeper) SortFlows() []*Flow {
	ret := make([]*Flow, 0, len(self.flows))
	for _, flow := range self.flows {
		ret = append(ret, flow)
	}

	sort.Sort(Flows(ret))
	return ret
}

func (self *FlowKeeper) add(flow *Flow) {
	assert(flow.lane < Nlane)
	flow.id = self.flowSeq
	flow.tstart = clock.T
	self.flows[self.flowSeq] = flow
	self.FlowCount[flow.lane]++
	self.flowSeq++
}

func (self *FlowKeeper) addSimple(lane int, size uint64) {
	self.add(NewSimpleFlow(lane, size))
}

func (self *FlowKeeper) removeFlow(id uint64) {
	flow := self.flows[id]
	if flow.closer != nil {
		flow.closer.Close(flow)
	}

	delete(self.flows, id)
}

func (self *FlowKeeper) updateTargetRate() {
	self.TargetRate = self.drainer.FlowRate(self.budget, self.FlowCount)
}

func (self *FlowKeeper) updateFlowRate() {
	for _, flow := range self.flows {
		flow.updateRate(self.TargetRate[flow.lane])
	}
}

func (self *FlowKeeper) sendDemand(sender Sender) {
	flows := self.SortFlows()

	for _, flow := range flows {
		size := flow.drain()
		if size == 0 {
			continue
		}

		pack := NewFlowPacket(size, flow.id, flow.lane)
		pack.Hint = flow.hint
		sent := sender.Send(pack)
		isActive := flow.putBack(size - sent)
		if !isActive {
			self.FlowCount[flow.lane]-- // not active anymore
		}
	}
}

func (self *FlowKeeper) Tick(sender Sender) {
	if self.driver != nil {
		self.driver.tick()
	}

	self.updateTargetRate()
	self.updateFlowRate()
	self.sendDemand(sender)

	// logln(self.TargetRate, "// TargetRate")
}

func (self *FlowKeeper) Push(packet *Packet) {
	id := packet.Flow
	flow := self.flows[id]
	assert(flow.lane == packet.Lane)
	if packet.DroppedBy == Nowhere {
		flow.acked += packet.Size

		if packet.ViaPacket {
			flow.viaPacket += packet.Size
		} else {
			flow.viaCircuit += packet.Size
		}

		if flow.ended() {
			self.removeFlow(id)
		}
	} else {
		flow.dropped += packet.Size

		wasActive := flow.isActive()
		flow.sent -= packet.Size // need to resend those
		if !wasActive {
			self.FlowCount[flow.lane]++ // bring back to active
		}
	}
}

// Hook a driver, this is just for ticking the driver on each time elapse
func (self *FlowKeeper) AssignDriver(driver *FlowDriver) {
	self.driver = driver
	driver.Keeper = self
}
