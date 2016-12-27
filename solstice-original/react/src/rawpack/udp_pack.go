package rawpack

import (
	"bytes"
	"net"
)

type UDPPacket struct {
	SrcMAC  net.HardwareAddr
	SrcIP   net.IP
	SrcPort uint16

	DestMAC  net.HardwareAddr
	DestPort uint16
	DestIP   net.IP
}

func (self *UDPPacket) _make(n int) *bytes.Buffer {
	buf := new(bytes.Buffer)

	eth := &EthHeader{
		SrcMAC:  self.SrcMAC,
		DestMAC: self.DestMAC,
		EthType: ipEthType,
	}

	ip := &IPHeader{
		Id:       defaultIPId,
		SrcIP:    self.SrcIP,
		DestIP:   self.DestIP,
		TTL:      defaultTTL,
		Protocol: udpProto,
	}

	udp := &UDPHeader{
		SrcPort:  self.SrcPort,
		DestPort: self.DestPort,
	}

	eth.Write(buf)
	ip.Write(buf, n+udpLen)
	udp.Write(buf, n)

	return buf
}

func (self *UDPPacket) MakeHeader(n int) []byte {
	return self._make(n).Bytes()
}

func (self *UDPPacket) Make(payload []byte) []byte {
	buf := self._make(len(payload))
	buf.Write(payload)
	Pad(buf)

	return buf.Bytes()
}
