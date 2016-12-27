package main

import (
	"net"
)

type Sink struct {
	conn    *net.TCPConn
	buf     []byte
	counter *Counter
}

func NewSink(conn *net.TCPConn, counter *Counter) *Sink {
	ret := new(Sink)
	ret.conn = conn
	ret.buf = make([]byte, parseBufSize(bufSize))
	assert(counter != nil)
	ret.counter = counter

	return ret
}

func (self *Sink) Serve() {
	// a sink will just serve in slience, and counting incoming bytes

	for running() {
		n, err := self.conn.Read(self.buf)
		if err != nil {
			break
		}
		self.counter.Count(n)
	}

	self.conn.Close()
}
