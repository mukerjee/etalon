package pqueues

import (
	"react/sim/clock"
	"react/sim/queues"
)

// A FIFO queue of packets
// inherits all the methods of queues.Queue
type flowQueue struct {
	Flow        uint64
	lastTouched uint64
	*queues.Queue
}

func newFlowQueue(flow uint64) *flowQueue {
	ret := new(flowQueue)
	ret.Flow = flow
	ret.Queue = queues.NewQueue()
	return ret
}

func (self *flowQueue) touch() {
	self.lastTouched = clock.T
}
