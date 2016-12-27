package ctrlh

import (
	"bytes"

	"rawpack"
)

type Packet struct {
	*bytes.Buffer
}

var BroadcastMAC = rawpack.BroadcastMAC

const ethTypeIP uint16 = 0x0800

func NewPacket() *Packet {
	ret := new(Packet)
	ret.Buffer = new(bytes.Buffer)
	return ret
}

func NewRoutePacketExtra(r byte, extra byte) *Packet {
	ret := NewPacket()

	ret.Write(BroadcastMAC)
	ret.Byte(r)
	ret.Byte(extra)
	ret.Pad(len(BroadcastMAC) - 2)
	ret.Uint16BE(ethTypeIP)

	return ret
}

func NewRoutePacket(r byte) *Packet {
	return NewRoutePacketExtra(r, 0)
}

func NewCtrlPacketExtra(cmd byte, extra byte) *Packet {
	ret := NewRoutePacket(routeCtrl)
	ret.Byte(cmd)
	ret.Byte(extra)

	return ret
}

func NewWeeksigPosition(tick uint64) *Packet {
	p := NewCtrlPacket(cmdWeeksig)
	p.Uint64(TickToCycle(tick))

	return p
}

func NewSchedSwap(nday int, loadStart bool) *Packet {
	p := NewCtrlPacketExtra(cmdSchedSwap, byte(nday))
	if loadStart {
		p.Uint64(1)
	} else {
		p.Uint64(0)
	}
	return p
}

func NewCtrlPacket(cmd byte) *Packet {
	return NewCtrlPacketExtra(cmd, 0)
}

func (self *Packet) Pad(n int) {
	self.Write(make([]byte, n))
}

func (self *Packet) Uint16(n uint16) {
	self.Byte(byte(n))
	self.Byte(byte(n >> 8))
}

func (self *Packet) Uint16BE(n uint16) {
	self.Byte(byte(n >> 8))
	self.Byte(byte(n))
}

func (self *Packet) Uint32(n uint32) {
	self.Byte(byte(n))
	self.Byte(byte(n >> 8))
	self.Byte(byte(n >> 16))
	self.Byte(byte(n >> 24))
}

func (self *Packet) Uint32BE(n uint32) {
	self.Byte(byte(n >> 24))
	self.Byte(byte(n >> 16))
	self.Byte(byte(n >> 8))
	self.Byte(byte(n))
}

func (self *Packet) Uint64(n uint64) {
	self.Byte(byte(n))
	self.Byte(byte(n >> 8))
	self.Byte(byte(n >> 16))
	self.Byte(byte(n >> 24))
	self.Byte(byte(n >> 32))
	self.Byte(byte(n >> 40))
	self.Byte(byte(n >> 48))
	self.Byte(byte(n >> 56))
}

func (self *Packet) Byte(b byte) {
	self.WriteByte(b)
}

func (self *Packet) Bytes(bs []byte) {
	self.Write(bs)
}

const MinPacketLen = 60
const MaxPacketLen = 1500

func (self *Packet) Pack() []byte {
	n := self.Len()
	if n < MinPacketLen {
		self.Pad(MinPacketLen - n)
	}

	return self.Buffer.Bytes()
}
