package main

import (
	"fmt"
	"net"
	"os"

	"react/util/hosts"
)

type Server struct {
	conn *net.UDPConn
	buf  []byte
}

func NewServer(host string) *Server {
	ret := new(Server)
	dest := fmt.Sprintf("%s:%d", host, port)
	addr, err := net.ResolveUDPAddr("udp4", dest)
	noErr(err)

	ret.conn, err = net.ListenUDP("udp4", addr)
	noErr(err)

	ret.buf = make([]byte, packetLen)

	return ret
}

func (self *Server) recv() ([]byte, *net.UDPAddr) {
	n, raddr, err := self.conn.ReadFromUDP(self.buf)
	noErr(err)

	return self.buf[:n], raddr
}

func mapSource(ip string) string {
	for i := 0; i < 8; i++ {
		if ip == fmt.Sprintf("192.168.1.5%d", i) {
			return fmt.Sprintf("r%d", i)
		}
	}

	return ip
}

type slot struct {
	counter *Counter
	index   int
}

func makeSlotMap(clients hosts.Hosts) map[string]*slot {
	ret := make(map[string]*slot)

	for i, c := range clients {
		_, in := ret[c.IP]
		assert(!in)

		ret[c.IP] = &slot{NewCounter(c.Name), i}
	}

	return ret
}

func (self *Server) serve(closed <-chan os.Signal, clients hosts.Hosts) {
	reporter := NewReporter(len(clients), closed)
	slotMap := makeSlotMap(clients)

	for len(closed) == 0 {
		p, raddr := self.recv()
		if p != nil {
			slot := slotMap[raddr.IP.String()]
			if slot == nil {
				continue
			}

			stat := slot.counter.Parse(p)
			if stat != nil {
				stat.SetIndex(slot.index)
				reporter.Update(stat)
			}
		}
	}
}
