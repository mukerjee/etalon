package queues

import (
	"math"

	. "react/sim/blocks"
	. "react/sim/config"
	. "react/sim/packet"
	. "react/sim/structs"
)

type Queues struct {
	queues   []*Queue
	size     Matrix
	capacity Matrix
}

var _ Block = new(Queues)
var _ Sender = new(Queues)

func NewQueues() *Queues {
	return NewSizedQueues(math.MaxUint64)
}

func NewSizedQueues(capacity uint64) *Queues {
	ret := new(Queues)
	ret.queues = make([]*Queue, Nlane)
	for i := 0; i < Nlane; i++ {
		ret.queues[i] = NewSizedQueue(capacity)
	}

	ret.size = NewMatrix()

	return ret
}

func NewNicQueues(capacity, totalCap uint64) *Queues {
	ret := NewSizedQueues(capacity)
	for src := 0; src < Nhost; src++ {
		allocator := NewAllocator(totalCap)
		for dest := 0; dest < Nhost; dest++ {
			ret.queues[Lane(src, dest)].Allocator = allocator
		}
	}

	return ret
}

func (self *Queues) updateLen() {
	for i := 0; i < Nlane; i++ {
		self.size[i] = self.queues[i].size
	}
}

func (self *Queues) Len() Matrix {
	self.updateLen()
	return self.size
}

func (self *Queues) LenTo(ret Matrix) {
	// ret.Clear()
	for i := 0; i < Nlane; i++ {
		ret[i] = self.queues[i].size
	}
}

func (self *Queues) Send(packet *Packet) uint64 {
	// self.Len()
	return self.queues[packet.Lane].Send(packet)
}

func (self *Queues) Push(packet *Packet) {
	self.queues[packet.Lane].Push(packet)
}

func (self *Queues) pullFrom(listEnd listEnd, size Matrix,
	next Block, loc, dropBy int, ret Matrix) {
	for i := 0; i < Nlane; i++ {
		r := self.queues[i].pullFrom(listEnd, size[i], next, loc, dropBy)
		if ret != nil {
			ret[i] = r
		}
	}

	self.updateLen()
}

func (self *Queues) pull(size Matrix, next Block,
	loc, dropBy int, ret Matrix) {
	self.pullFrom(listFront, size, next, loc, dropBy, ret)
}

func (self *Queues) Cut(size Matrix, next Block,
	loc, dropBy int, ret Matrix) {
	self.pullFrom(listBack, size, next, loc, dropBy, ret)
}

func (self *Queues) CutToCapacity(next Block, dropBy int, ret Matrix) {
	for i := 0; i < Nlane; i++ {
		r := self.queues[i].cutToCapacity(next, dropBy)
		if ret != nil {
			ret[i] = r
		}
	}

	self.updateLen()
}

func (self *Queues) Move(size Matrix, next Block, loc int, ret Matrix) {
	self.pull(size, next, loc, Nowhere, ret)
}

func (self *Queues) MoveAll(next Block, loc int, ret Matrix) {
	self.Move(self.Len(), next, loc, ret)
}

func (self *Queues) Drop(size Matrix, next Block, dropBy int, ret Matrix) {
	self.pull(size, next, Dropped, dropBy, ret)
}

func (self *Queues) DropAll(next Block, dropBy int, ret Matrix) {
	self.pull(self.Len(), next, Dropped, dropBy, ret)
}

func (self *Queues) Clear() {
	for i := 0; i < Nlane; i++ {
		self.queues[i].clear()
	}
	self.size.Clear()
}

func (self *Queues) MoveUsing(next Block, loc int,
	puller Puller) (buf Matrix, pulled Matrix) {
	buf = self.Len()
	pulled = puller.Pull(buf)
	self.Move(pulled, next, loc, nil)

	return buf, pulled
}
