package rawpack

import (
	"bytes"
	"encoding/binary"
	"net"
)

type EthHeader struct {
	SrcMAC  net.HardwareAddr
	DestMAC net.HardwareAddr
	EthType uint16
}

func (self *EthHeader) Write(buf *bytes.Buffer) {
	buf.Write(self.DestMAC)
	buf.Write(self.SrcMAC)
	binary.Write(buf, enc, self.EthType)
}
