package headq

import (
	// "log"
	"react/testb/setup"
)

type idChecker struct {
	c       string
	expects []uint32
}

func newIdChecker(c string) *idChecker {
	ret := new(idChecker)
	ret.c = c
	ret.expects = make([]uint32, setup.Nlane)
	return ret
}

func (self *idChecker) CheckId(d *Dump) {
	lane := d.Lane()
	expect := self.expects[lane]
	this := d.SeqNo
	// log.Printf("lane: %d %d-%d id=%d", d.Lane(), d.From, d.To, d.SeqNo)
	if expect != this && expect != this+1 {
		/*
			if expect+1 == this {
				log.Printf("%s %d-%d #%d lost", self.c,
					d.From, d.To, expect)
			} else {
				log.Printf("%s %d-%d #%d-%d lost", self.c,
					d.From, d.To, expect, this-1)
			}
		*/
	}
	self.expects[lane] = this + 1
}
