package sim

import (
	"encoding/json"
	"fmt"
	"io"

	. "react/sim/blocks"
	"react/sim/clock"
	. "react/sim/packet"
	. "react/sim/structs"
)

type exit struct {
	Goodput uint64
	Circput uint64
	Packput uint64
	Dropped uint64
	Starts  uint64

	MiceCirc uint64
	MicePack uint64
	MiceDrop uint64
	EleCirc  uint64
	ElePack  uint64
	EleDrop  uint64

	recv *Counter
	circ *Counter
	pack *Counter
	drop *Counter

	sink Block
}

var _ Block = new(exit)

func newExit(sink Block) *exit {
	ret := new(exit)
	ret.sink = sink

	ret.recv = NewCounter()
	ret.circ = NewCounter()
	ret.pack = NewCounter()
	ret.drop = NewCounter()

	return ret
}

func (self *exit) ClearSum() {
	self.recv.ClearSum()
	self.circ.ClearSum()
	self.pack.ClearSum()
	self.drop.ClearSum()
}

func (self *exit) Clear() {
	self.recv.Clear()
	self.circ.Clear()
	self.pack.Clear()
	self.drop.Clear()
}

func (self *exit) Push(p *Packet) {
	if p.DroppedBy == Nowhere {
		if clock.T >= self.Starts {
			self.Goodput += p.Size
		}
		self.recv.CountPacket(p)
	} else {
		if clock.T >= self.Starts {
			self.Dropped += p.Size
		}
		self.drop.CountPacket(p)
	}

	if p.Hint == Elephant {
		if p.DroppedBy == Nowhere {
			if p.ViaPacket {
				self.ElePack += p.Size
			} else {
				self.EleCirc += p.Size
			}
		} else {
			self.EleDrop += p.Size
		}
	} else if p.Hint == Mouse {
		if p.DroppedBy == Nowhere {
			if p.ViaPacket {
				self.MicePack += p.Size
			} else {
				self.MiceCirc += p.Size
			}
		} else {
			self.MiceDrop += p.Size
		}
	}

	if self.sink != nil {
		self.sink.Push(p)
	}

	// FIXME: add a logger here
	if p.Flow == 7115 {
		fmt.Println(p)
		for _, fp := range p.Footprint {
			fmt.Println(fp)
		}
	}
}

func (self *exit) CountServed(circ Matrix, pack Matrix) {
	self.circ.CountMatrix(circ)
	self.pack.CountMatrix(pack)

	if clock.T >= self.Starts {
		if circ != nil {
			self.Circput += circ.Sum()
		}
		if pack != nil {
			self.Packput += pack.Sum()
		}
	}
}

func (self *exit) Summarize(out io.Writer) error {
	var sum struct {
		Good, Circ, Pack, Drop       uint64
		EleCirc, ElePack, EleDrop    uint64
		MiceCirc, MicePack, MiceDrop uint64
	}
	sum.Good = self.Goodput
	sum.Circ = self.Circput
	sum.Pack = self.Packput
	sum.Drop = self.Dropped

	sum.EleCirc = self.EleCirc
	sum.ElePack = self.ElePack
	sum.EleDrop = self.EleDrop

	sum.MiceCirc = self.MiceCirc
	sum.MicePack = self.MicePack
	sum.MiceDrop = self.MiceDrop

	bs, e := json.MarshalIndent(&sum, "", "  ")
	if e != nil {
		return e
	}

	_, e = out.Write(bs)
	return e
}
