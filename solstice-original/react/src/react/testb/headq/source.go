package headq

import (
	"log"
)

type source struct {
	d Dumper
	*idChecker
}

func newSource(d Dumper) *source {
	ret := new(source)
	ret.d = d
	ret.idChecker = newIdChecker("send")

	return ret
}

func (self *source) Check() bool { return self.d.Check() }

func (self *source) Dump() *Dump {
	d := self.d.Dump()

	if d.Chan != int(d.From) {
		log.Printf("packet from %d on src chan %d", d.From, d.Chan)
	}

	self.CheckId(d)

	return d
}
