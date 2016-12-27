package main

import (
	"fmt"
	"net"
	"react/util/hosts"
)

type Server struct {
	conn     *net.TCPListener
	counters []*Counter
	slotMap  map[string]*slot
}

func makeSlotMap(clients hosts.Hosts) map[string]*slot {
	ret := make(map[string]*slot)

	for i, c := range clients {
		_, in := ret[c.IP]
		assert(!in)

		ret[c.IP] = &slot{c.Name, i}
	}

	return ret
}

func NewServer(host string, clients hosts.Hosts) *Server {
	ret := new(Server)
	dest := fmt.Sprintf("%s:%d", host, port)
	addr, err := net.ResolveTCPAddr("tcp4", dest)
	noErr(err)

	ret.conn, err = net.ListenTCP("tcp4", addr)
	noErr(err)

	ret.counters = make([]*Counter, len(clients))
	for i, c := range clients {
		ret.counters[i] = NewCounter(c.Name, i)
	}

	ret.slotMap = makeSlotMap(clients)

	return ret
}

type slot struct {
	name  string
	index int
}

func (self *Server) Serve() {
	for running() {
		conn, err := self.conn.AcceptTCP()
		noErr(err)

		hostport := conn.RemoteAddr().String()
		host, _, err := net.SplitHostPort(hostport)
		noErr(err)

		slot := self.slotMap[host]
		if slot == nil {
			// conn not welcomed
			// fmt.Printf("%s not welcomed\n", host)
			conn.Close()
			continue
		}

		sink := NewSink(conn, self.counters[slot.index])

		go sink.Serve()
	}
}
