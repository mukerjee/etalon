package rawpack

import (
	"bytes"
	"encoding/binary"
	"net"
)

type IPHeader struct {
	Id       uint16
	SrcIP    net.IP
	DestIP   net.IP
	TTL      uint8
	Protocol uint8
}

func ipCheckAdd(sum uint64, buf []byte) uint64 {
	if len(buf)%2 != 0 {
		panic("buf must align to 16 bits")
	}

	for i := 0; i < len(buf); i += 2 {
		sum += uint64(enc.Uint16(buf[i : i+2]))
	}

	return sum
}

func ipCheckFold(sum uint64) uint16 {
	return ^(uint16(sum) + uint16(sum>>16))
}

func (self *IPHeader) Write(out *bytes.Buffer, n int) {
	buf := new(bytes.Buffer)

	buf.WriteByte(0x45) // version, ihl
	buf.WriteByte(0x00) // DSCP, ECN
	_n := uint16(n + ipLen)
	binary.Write(buf, enc, _n)
	binary.Write(buf, enc, self.Id)
	binary.Write(buf, enc, uint16(0)) // fragmentation
	buf.WriteByte(self.TTL)
	buf.WriteByte(self.Protocol)

	sum := ipCheckAdd(0, buf.Bytes())
	sum = ipCheckAdd(sum, self.SrcIP)
	sum = ipCheckAdd(sum, self.DestIP)

	out.Write(buf.Bytes())
	binary.Write(out, enc, ipCheckFold(sum))
	out.Write(self.SrcIP)
	out.Write(self.DestIP)
}
