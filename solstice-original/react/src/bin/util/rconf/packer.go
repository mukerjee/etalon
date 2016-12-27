package main

import (
	"encoding/binary"
)

var encoder = binary.LittleEndian

type Packer struct {
	buf  []byte
	size int
}

const maxPackSize = 1500
const minPackSize = 60

func NewPacker() *Packer {
	ret := new(Packer)
	ret.buf = make([]byte, maxPackSize)
	ret.size = 0

	return ret
}

func (self *Packer) next(n int) []byte {
	size := self.size
	assert(size+n <= maxPackSize)
	ret := self.buf[size : size+n]
	self.size += n
	return ret
}

func (self *Packer) u64(i uint64) {
	encoder.PutUint64(self.next(8), i)
}

func (self *Packer) u32(i uint32) {
	encoder.PutUint32(self.next(4), i)
}

func (self *Packer) u16(i uint16) {
	encoder.PutUint16(self.next(2), i)
}

func (self *Packer) u8(i uint8) {
	self.next(1)[0] = byte(i)
}

func (self *Packer) c(i uint8) {
	self.u8(i)
}

func (self *Packer) pad(n int) {
	buf := self.next(n)
	for i := range buf {
		buf[i] = 0
	}
}

func (self *Packer) sendVia(comm Comm) {
	if self.size < minPackSize {
		self.pad(minPackSize - self.size)
	}

	comm.Send(self.buf[0:self.size])
}

func (self *Packer) clear() {
	self.size = 0
}

func (self *Packer) put(s []byte) {
	copy(self.next(len(s)), s)
}
