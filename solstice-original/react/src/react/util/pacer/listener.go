package pacer

import (
	"net"
)

type Listener struct {
	conn *net.UDPConn
	buf  []byte
}

const MaxMsgLen = 65536

func NewListener() *Listener {
	ret := new(Listener)

	var err error
	ret.conn, err = net.ListenUDP("udp4", broadcastAddr)
	noErr(err)
	ret.buf = make([]byte, MaxMsgLen)

	return ret
}

func (self *Listener) Recv() []byte {
	n, err := self.conn.Read(self.buf)
	noErr(err)

	return self.buf[:n]
}
