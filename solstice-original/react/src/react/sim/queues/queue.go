package queues

import (
	"container/list"
	"math"
	. "react/sim/blocks"
	. "react/sim/config"
	. "react/sim/packet"
)

type Queue struct {
	buf    *list.List
	single *Packet

	capacity uint64
	size     uint64

	BackPressure bool

	Allocator *Allocator
}

var _ Block = new(Queue)
var _ Sender = new(Queue)

func NewQueue() *Queue {
	return NewSizedQueue(math.MaxUint64)
}

func NewSizedQueue(capacity uint64) *Queue {
	ret := new(Queue)
	if Tracking {
		ret.buf = list.New()
	}
	ret.capacity = capacity
	ret.BackPressure = true
	return ret
}

func (self *Queue) Size() uint64 {
	return self.size
}

func (self *Queue) Send(packet *Packet) uint64 {
	if self.capacity <= self.size {
		return 0
	}

	if self.capacity < self.size+packet.Size {
		packet.Size = self.capacity - self.size
	} else if self.BackPressure {
		if self.size > self.capacity*3/4 && packet.Size >= 10 {
			packet.Size *= 9
			packet.Size /= 10
		}
	}

	if self.Allocator != nil {
		packet.Size = self.Allocator.Request(packet.Size)
		if packet.Size == 0 {
			return 0
		}
	}

	packet.Pin(Source)

	self.Push(packet)
	return packet.Size
}

func (self *Queue) Push(packet *Packet) {
	if self.Allocator != nil {
		size := self.Allocator.Alloc(packet.Size)
		assert(size == packet.Size)
	}

	if Tracking {
		self.buf.PushBack(packet)
		self.size += packet.Size
	} else {
		if self.single == nil {
			self.single = packet
		} else {
			assert(self.single.Flow == packet.Flow)
			assert(self.single.Lane == packet.Lane)
			assert(self.single.DroppedBy == packet.DroppedBy)

			self.single.Size += packet.Size
		}
		self.size = self.single.Size
	}
}

func listBack(lst *list.List) *list.Element  { return lst.Back() }
func listFront(lst *list.List) *list.Element { return lst.Front() }

type listEnd func(*list.List) *list.Element

func (self *Queue) pullFrom(listEnd listEnd, size uint64,
	next Block, loc, dropBy int) uint64 {
	if Tracking {
		budget := size
		for budget > 0 {
			e := listEnd(self.buf)
			if e == nil {
				break
			}

			p := e.Value.(*Packet)
			if p.Size > budget {
				p = p.Split(budget)
			} else {
				self.buf.Remove(e)
			}

			assert(self.size >= p.Size)
			assert(budget >= p.Size)
			budget -= p.Size
			self.size -= p.Size
			p.Mark(loc, dropBy)
			if next != nil {
				next.Push(p)
			}
		}

		if self.Allocator != nil {
			self.Allocator.Free(size - budget)
		}

		return size - budget
	} else {
		p := self.single
		if p == nil || size == 0 {
			return 0
		}

		if p.Size > size {
			p = p.Split(size)
		} else {
			self.single = nil
		}

		assert(self.size >= p.Size)
		self.size -= p.Size
		p.Mark(loc, dropBy)

		if next != nil {
			next.Push(p)
		}

		if self.Allocator != nil {
			self.Allocator.Free(p.Size)
		}

		return p.Size
	}
}

func (self *Queue) pull(size uint64, next Block, loc, dropBy int) uint64 {
	return self.pullFrom(listFront, size, next, loc, dropBy)
}

func (self *Queue) Pull(size uint64, next Block, loc, dropBy int) uint64 {
	return self.pull(size, next, loc, dropBy)
}

func (self *Queue) cut(size uint64, next Block, loc, dropBy int) uint64 {
	return self.pullFrom(listBack, size, next, loc, dropBy)
}

func (self *Queue) cutToCapacity(next Block, dropBy int) uint64 {
	if self.size <= self.capacity {
		return 0
	}
	return self.cut(self.capacity-self.size, next, Dropped, dropBy)
}

func (self *Queue) move(size uint64, next Block, loc int) uint64 {
	assert(loc != Dropped)
	return self.pull(size, next, loc, Nowhere)
}

func (self *Queue) moveAll(next Block, loc int) uint64 {
	return self.move(self.size, next, loc)
}

func (self *Queue) drop(size uint64, next Block, dropBy int) uint64 {
	return self.pull(size, next, Dropped, dropBy)
}

func (self *Queue) dropAll(next Block, dropBy int) uint64 {
	return self.pull(self.size, next, Dropped, dropBy)
}

func (self *Queue) clear() {
	if Tracking {
		for {
			e := self.buf.Front()
			if e == nil {
				break
			}
			self.buf.Remove(e)
		}
	} else {
		self.single = nil
	}

	self.size = 0
}
