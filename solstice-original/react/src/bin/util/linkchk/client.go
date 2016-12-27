package main

import (
	"fmt"
	"net"
)

type Client struct {
	addr  *net.UDPAddr
	conn  *net.UDPConn
	buf   []byte
	group uint64
	id    uint64
}

func NewClient(host string) *Client {
	ret := new(Client)

	var err error
	dest := fmt.Sprintf("%s:%d", host, port)
	ret.addr, err = net.ResolveUDPAddr("udp4", dest)
	noErr(err)

	ret.buf = make([]byte, packetLen)
	encoder.PutUint64(ret.buf[0:8], SIG)

	return ret
}

func (self *Client) Tick() {
	var err error

	if self.conn == nil {
		self.conn, err = net.DialUDP("udp4", nil, self.addr)
		if err != nil {
			self.conn = nil
		} else {
			self.group = 0
			self.id = 0
		}
	}

	if self.conn != nil {
		encoder.PutUint64(self.buf[8:16], self.group)
		encoder.PutUint64(self.buf[16:24], self.id)
		encoder.PutUint64(self.buf[24:32], perGroup)

		written, err := self.conn.Write(self.buf)
		if err != nil {
			self.conn = nil
			return
		}
		assert(written == packetLen)

		self.id++
		if self.id == perGroup {
			self.id = 0
			self.group++
			// fmt.Println("group:", self.group)
		}
	}
}
