package pacer

import (
	"net"
)

type Sender struct {
	conn *net.UDPConn
}

func NewSender() *Sender {
	ret := new(Sender)

	var err error
	ret.conn, err = net.DialUDP("udp4", nil, broadcastAddr)
	noErr(err)

	return ret
}

func (self *Sender) Broadcast(msg []byte) {
	written, err := self.conn.Write(msg)
	noErr(err)
	assert(written == len(msg))
}
