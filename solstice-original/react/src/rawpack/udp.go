package rawpack

import (
	"bytes"
	"encoding/binary"
)

type UDPHeader struct {
	SrcPort  uint16
	DestPort uint16
}

func (self *UDPHeader) Write(buf *bytes.Buffer, n int) {
	_n := uint16(n + udpLen)

	binary.Write(buf, enc, self.SrcPort)
	binary.Write(buf, enc, self.DestPort)
	binary.Write(buf, enc, _n)
	binary.Write(buf, enc, uint16(0)) // check sum
}
