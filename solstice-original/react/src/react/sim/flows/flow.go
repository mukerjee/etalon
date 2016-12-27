package flows

import "fmt"

// flow class
const (
	SimpleFlow = iota
	Background
	QueryRequest
	QueryRespond
)

type Flow struct {
	id      uint64
	lane    int
	size    uint64
	class   int
	hint    int
	closer  FlowCloser
	tstart  uint64
	Payload uint64

	sent  uint64
	acked uint64

	rate   uint64
	stable bool

	viaPacket  uint64
	viaCircuit uint64
	dropped    uint64
}

type FlowCloser interface {
	Close(*Flow)
}

var EnableSlowStart = false

func NewFlow(lane int, size uint64, class int, closer FlowCloser) *Flow {
	ret := new(Flow)
	ret.lane = lane
	ret.size = size
	ret.class = class
	ret.closer = closer

	return ret
}

func NewSimpleFlow(lane int, size uint64) *Flow {
	return NewFlow(lane, size, SimpleFlow, nil)
}

func (self *Flow) String() string {
	return fmt.Sprintf("id=%d lane=%d size=%d class=%d tstart=%d",
		self.id, self.lane, self.size, self.class, self.tstart)
}

var stableRateDelta = uint64(1)
var slowStartFactor = float64(1.3)
var oracleRate = true

func (self *Flow) updateRate(targetRate uint64) {
	if oracleRate {
		self.rate = targetRate
		self.stable = true
		return
	}

	if self.rate <= targetRate {
		if self.stable {
			self.rate += stableRateDelta
		} else {
			if EnableSlowStart {
				nextRate := (float64)(self.rate) * slowStartFactor
				self.rate = (uint64)(nextRate) + 1
			} else {
				self.rate = targetRate
			}
		}

		if self.rate > targetRate {
			// will nver overflow
			self.rate = targetRate
			self.stable = true
		}
	} else {
		self.rate /= 2
		self.stable = true
	}
}

func (self *Flow) drain() uint64 {
	assert(self.sent <= self.size)
	delta := self.size - self.sent
	if delta > self.rate {
		delta = self.rate
	}

	self.sent += delta
	return delta
}

func (self *Flow) ended() bool {
	assert(self.acked <= self.sent)
	assert(self.sent <= self.size)
	return self.acked == self.size
}

func (self *Flow) putBack(n uint64) bool {
	assert(n <= self.sent)
	self.sent -= n
	return self.isActive()
}

func (self *Flow) isActive() bool {
	return self.sent < self.size
}
