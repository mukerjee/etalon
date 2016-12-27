package headq

import (
	"bufio"
	"io"
	"log"
)

type Dumper interface {
	Check() bool // auto closes on last check
	Dump() *Dump
}

type dumper struct {
	r io.ReadCloser
	s *bufio.Scanner
	d *Dump
	c int
}

func NewDumper(r io.ReadCloser) *dumper {
	return NewChanDumper(r, 0)
}

func NewChanDumper(r io.ReadCloser, c int) *dumper {
	ret := new(dumper)
	ret.r = r
	ret.s = bufio.NewScanner(r)
	ret.c = c

	return ret
}

var _ Dumper = new(dumper)

func (self *dumper) Check() bool {
	self.d = self.check()
	return self.d != nil
}

func (self *dumper) Dump() *Dump {
	return self.d
}

func (self *dumper) check() *Dump {
	if !self.s.Scan() {
		e := self.s.Err()
		if e != nil {
			log.Fatalln(e)
		}
		self.r.Close()
		return nil
	}

	ret, e := ParseDump(self.s.Text(), self.c)
	if e != nil {
		log.Fatalln(e)
	}

	return ret
}
