package tors

import (
	. "react/sim"
	. "react/sim/blocks"
	. "react/sim/config"
	. "react/sim/drainer"
	. "react/sim/packet"
	. "react/sim/queues"
	. "react/sim/structs"
)

type PacketSwitch struct {
	nics       *Queues
	packSwitch *Queues
	downraters *Queues

	NicQueue Matrix
	PackUp   Matrix
	PackBuf  Matrix
	PackDown Matrix
	DownThru Matrix

	PackBufDrop Matrix
	DownDrop    Matrix

	upDrainer       *LinkDrainer
	packDownDrainer *LinkDrainer
	downDrainer     *LinkDrainer
}

var _ Switch = new(PacketSwitch)

func NewPacketSwitch() *PacketSwitch {
	ret := new(PacketSwitch)

	ret.nics = NewNics()
	ret.packSwitch = NewSizedQueues(SwitchBufSize())
	ret.downraters = NewQueues()

	// XXX: not sure if this packDownDrainer is setup correctly
	ret.upDrainer = NewUplinkDrainer(LinkBw)
	ret.packDownDrainer = NewUplinkDrainer(LinkBw + PackBw)
	ret.downDrainer = NewDownlinkDrainer(LinkBw)

	// NicQueue <= nics
	// PackUp <= upDrainer
	// PackBuf <= packSwitch
	// PackDown <= packDownDrainer
	// DownThru <= downDrainer
	ret.DownDrop = NewMatrix()
	ret.PackBufDrop = NewMatrix()

	return ret
}

func (self *PacketSwitch) Send(packet *Packet) uint64 {
	return self.nics.Send(packet)
}

func (s *PacketSwitch) Tick(sink Block, _ Estimator) (Matrix, Events) {
	// nic buffer
	s.NicQueue, s.PackUp = s.nics.MoveUsing(s.packSwitch,
		SwitchBuffer, s.upDrainer)

	// downlink step 1: out of the switch
	s.PackBuf, s.PackDown = s.packSwitch.MoveUsing(s.downraters,
		DownMerger, s.packDownDrainer)

	// downlink step 2: down rating
	_, s.DownThru = s.downraters.MoveUsing(sink,
		Destination, s.downDrainer)
	s.downraters.DropAll(sink, Downlink, s.DownDrop)

	// packet buffer drop
	s.packSwitch.CutToCapacity(sink, PacketBuffer, s.PackBufDrop)

	return nil, 0
}

func (s *PacketSwitch) Tdma() bool                         { return false }
func (s *PacketSwitch) Bind(_ Scheduler, _ *SchedRecorder) {}
func (s *PacketSwitch) Served() (circ Matrix, pack Matrix) {
	return nil, s.DownThru
}
