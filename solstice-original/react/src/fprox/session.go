package fprox

import (
	"bufio"
	"errors"
	"io"
	"log"
	"net"
	"os"
)

type session struct {
	addr    string
	conn    *net.TCPConn
	reader  *bufio.Reader
	mapper  Mapper
	verbose bool
}

func serveSession(conn *net.TCPConn, m Mapper, v bool) {
	s := new(session)
	s.addr = conn.RemoteAddr().String()
	s.conn = conn
	s.mapper = m
	s.verbose = v

	s.serve()
}

func (self *session) log(e error) {
	log.Println(self.addr, e)
}

func (self *session) serve() {
	self.reader = bufio.NewReader(self.conn)
	defer func() {
		e := self.conn.Close()
		if e != nil {
			self.log(e)
		}
	}()

	cmd, e := self.reader.ReadString('\n')
	if e != nil {
		self.log(e)
		return
	}
	cmd = cmd[:len(cmd)-1]

	path, e := self.reader.ReadString('\n')
	if e != nil {
		self.log(e)
		return
	}
	path = path[:len(path)-1]

	if cmd == "read" {
		path, e = self.mapper.MapRead(path)
		if e != nil {
			self.log(e)
			return
		}

		self.read(path)
	} else if cmd == "write" {
		path, e = self.mapper.MapWrite(path)
		if e != nil {
			self.log(e)
			return
		}

		self.write(path)
		_, e = self.conn.Write([]byte{0})
		if e != nil {
			self.log(e)
			return
		}
	} else {
		self.log(errors.New("invalid command"))
	}
}

func (self *session) read(path string) {
	if self.verbose {
		log.Println("read", path)
	}

	f, e := os.Open(path)
	if e != nil {
		self.log(e)
		return
	}
	defer func() {
		e := f.Close()
		if e != nil {
			self.log(e)
		}
	}()

	_, e = io.Copy(self.conn, f)
	if e != nil {
		self.log(e)
		return
	}
}

func (self *session) write(path string) {
	if self.verbose {
		log.Println("write", path)
	}

	f, e := os.Create(path)
	if e != nil {
		self.log(e)
		return
	}

	defer func() {
		e := f.Close()
		if e != nil {
			self.log(e)
			return
		}
	}()

	_, e = io.Copy(f, self.reader)
	if e != nil {
		self.log(e)
		return
	}
}
