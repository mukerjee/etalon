package headq

import (
	"log"
)

type sink struct {
	d Dumper
	*idChecker
}

func newSink(d Dumper) *sink {
	ret := new(sink)
	ret.d = d
	ret.idChecker = newIdChecker("recv")

	return ret
}

func (self *sink) Check() bool { return self.d.Check() }

func (self *sink) Dump() *Dump {
	d := self.d.Dump()
	if d.Chan != int(d.To) {
		log.Printf("packet to %d on sink chan %d id %d",
			d.To, d.Chan, d.SeqNo)
	}

	self.CheckId(d)

	return d
}
