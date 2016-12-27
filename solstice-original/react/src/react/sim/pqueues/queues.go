package pqueues

import (
	"react/sim/blocks"
	"react/sim/config"
	"react/sim/packet"
	"react/sim/queues"
	"react/sim/structs"
)

// A path queue that uses a flow heap
/*
	This is very similar to queues.Queues.  I probably should merge them
	with some abstraction, but a queue supports pulling from back and also
	dropping, so it generalizing that is a little bit complicated.

	I might generalize that later. For now, I need something that just works.
*/
type Queues struct {
	queues []*Queue
	size   structs.Matrix
}

func NewQueues() *Queues {
	ret := new(Queues)

	ret.queues = make([]*Queue, config.Nlane)
	for i := 0; i < config.Nlane; i++ {
		ret.queues[i] = NewQueue()
	}
	ret.size = structs.NewMatrix()

	return ret
}

func (self *Queues) updateLen() {
	for i := 0; i < config.Nlane; i++ {
		self.size[i] = self.queues[i].total
	}
}

func (self *Queues) Len() structs.Matrix {
	self.updateLen()
	return self.size
}

func (self *Queues) Send(p *packet.Packet) uint64 {
	return self.queues[p.Lane].Send(p)
}

func (self *Queues) Push(p *packet.Packet) {
	// TODO: reflect some back-pressure
	self.queues[p.Lane].Push(p)
}

func (self *Queues) pull(size structs.Matrix, next blocks.Block,
	loc, drop int, ret structs.Matrix) {
	for i := 0; i < config.Nlane; i++ {
		r := self.queues[i].Pull(size[i], next, loc, drop)
		if ret != nil {
			ret[i] = r
		}
	}

	self.updateLen()
}

func (self *Queues) Move(size structs.Matrix,
	next blocks.Block, loc int, ret structs.Matrix) {
	self.pull(size, next, loc, packet.Nowhere, ret)
}

func (self *Queues) MoveUsing(next blocks.Block, loc int,
	puller queues.Puller) (buf structs.Matrix, pulled structs.Matrix) {
	buf = self.Len()
	pulled = puller.Pull(buf)
	self.Move(pulled, next, loc, nil)

	return buf, pulled
}
