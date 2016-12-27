package pqueues

import (
	"container/heap"
	"react/sim/blocks"
	"react/sim/packet"
)

// A path queue that uses a flow heap
type Queue struct {
	h     *flowHeap
	total uint64
}

func NewQueue() *Queue {
	ret := new(Queue)
	ret.h = newFlowHeap()
	return ret
}

func (self *Queue) Send(p *packet.Packet) uint64 {
	// TODO: need some rethinking about this
	self.Push(p)
	return p.Size
}

// Push a packet into the queue
func (self *Queue) Push(p *packet.Packet) {
	index, found := self.h.flowMap[p.Flow]
	if found {
		// flow already exists
		q := self.h.flows[index]
		q.Push(p) // push the packet into the flow queue
		q.touch()
		heap.Fix(self.h, index) // fix the heap for future pulling
	} else {
		q := newFlowQueue(p.Flow) // create a new flow queue
		q.Push(p)                 // push the packet into the queue
		q.touch()
		heap.Push(self.h, q) // and push the queue into the heap
	}

	self.total += p.Size
}

// Pull n number of bytes, move them to block,
// mark it as moving to location "loc" on the packet trace
// and set dropBy to drop if the location is a drop
func (self *Queue) Pull(n uint64, next blocks.Block, loc, drop int) uint64 {
	budget := n
	for budget > 0 && self.total > 0 {
		assert(len(self.h.flows) > 0)
		top := self.h.flows[0]
		assert(top != nil)

		toPull := top.Size()
		assert(toPull <= self.total)

		if toPull > budget {
			toPull = budget
		} else {
			heap.Pop(self.h)
		}

		delta := top.Pull(toPull, next, loc, drop)
		assert(delta == toPull)

		budget -= delta
		self.total -= delta
	}

	return n - budget
}
