package fprox

import (
	"net"
)

type Server struct {
	Addr    string
	Mapper  Mapper
	Verbose bool
}

func (self *Server) Serve() error {
	addr, e := net.ResolveTCPAddr("tcp4", self.Addr)
	if e != nil {
		return e
	}

	lis, e := net.ListenTCP("tcp4", addr)
	if e != nil {
		return e
	}

	for {
		conn, e := lis.AcceptTCP()
		if e != nil {
			return e
		}

		go serveSession(conn, self.Mapper, self.Verbose)
	}
}
