package packet

import (
	"fmt"

	. "react/sim/config"
)

// DroppedBy's
const (
	Nowhere = iota
	Uplink
	Downlink
	PacketBuffer
)

// Hints
const (
	Unknown  = iota // not sure
	Elephant        // big flows
	Mouse           // small flows
)

type Packet struct {
	Size      uint64
	Flow      uint64
	Hint      int
	Lane      int
	DroppedBy int
	ViaPacket bool
	Footprint []*Footprint
}

func (self *Packet) String() string {
	if self.Flow == 0 {
		return fmt.Sprintf("[lane=%d size=%d]",
			self.Lane, self.Size)
	}
	return fmt.Sprintf("[lane=%d size=%d flow=%d viapack=%t]",
		self.Lane, self.Size, self.Flow, self.ViaPacket)
}

// we add lane so that we don't need to
// create a receiver for each lane

const footprintMaxCount = 5

func NewPacket(size uint64, lane int) *Packet {
	return NewFlowPacket(size, 0, lane)
}

func NewFlowPacket(size, flow uint64, lane int) *Packet {
	ret := new(Packet)
	ret.Size = size
	ret.Flow = flow
	ret.Lane = lane
	if Tracking {
		ret.Footprint = make([]*Footprint, 0, footprintMaxCount)
	}
	return ret
}

func (self *Packet) Pin(location int) {
	if Tracking {
		self.Footprint = append(self.Footprint, NewFootprint(location))
		if location == SwitchBuffer {
			self.ViaPacket = true
		}
	}
}

func (self *Packet) Drop(by int) {
	self.DroppedBy = by
}

func (self *Packet) Mark(location int, droppedBy int) {
	self.Pin(location)
	if location == Dropped {
		self.Drop(droppedBy)
	}
}

func (self *Packet) Split(size uint64) *Packet {
	assert(size < self.Size)
	assert(size > 0)

	self.Size -= size

	ret := NewFlowPacket(size, self.Flow, self.Lane)
	if Tracking {
		ret.Footprint = make([]*Footprint, len(self.Footprint))
		copy(ret.Footprint, self.Footprint)
	}
	ret.DroppedBy = self.DroppedBy
	ret.ViaPacket = self.ViaPacket
	ret.Hint = self.Hint

	return ret
}
