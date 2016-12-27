package headq

import (
	"react/replay"
	"react/testb/setup"
)

type DumpChan struct {
	dumper Dumper
	cur    replay.Matrix
	next1  replay.Matrix
	next2  replay.Matrix
}

func NewDumpChan(dumper Dumper) *DumpChan {
	ret := new(DumpChan)
	ret.dumper = dumper
	ret.cur = replay.NewMatrix(setup.Nhost)
	ret.next1 = replay.NewMatrix(setup.Nhost)
	ret.next2 = replay.NewMatrix(setup.Nhost)

	if !dumper.Check() {
		ret.dumper = nil // empty dumper
	}

	return ret
}

func (s *DumpChan) shift() {
	s.cur, s.next1, s.next2 = s.next1, s.next2, s.cur
	s.next2.Clear()
}

func (self *DumpChan) Tick(tick uint64) replay.Matrix {
	self.shift()

	if self.Closed() {
		return self.cur
	}

	return self.tick(tick)
}

func (self *DumpChan) tick(tick uint64) replay.Matrix {
	for {
		d := self.dumper.Dump()
		if d.Tick() > tick {
			break
		}

		self.add(d)

		closed := !self.dumper.Check()
		if closed {
			self.dumper = nil
			break
		}
	}

	return self.cur
}

func (self *DumpChan) add(d *Dump) {
	lane := d.Lane()
	self.cur[lane] += setup.PackSize
}

func (self *DumpChan) Closed() bool {
	return self.dumper == nil
}
