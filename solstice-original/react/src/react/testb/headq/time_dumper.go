package headq

import (
	"react/testb/setup"
)

// TimeDumper merges multiple Dumpers by Dump.T
type TimeDumper struct {
	dumpers []Dumper
	d       *Dump
}

func NewTimeDumper() *TimeDumper {
	ret := new(TimeDumper)
	ret.dumpers = make([]Dumper, setup.Nhost)

	return ret
}

func (self *TimeDumper) update(i int) {
	d := self.dumpers[i]

	if !d.Check() {
		self.dumpers[i] = nil // say goodbye
	}
}

func (self *TimeDumper) Add(i int, d Dumper) {
	self.dumpers[i] = d
	self.update(i)
}

func (self *TimeDumper) Check() bool {
	self.d = self.check()
	return self.d != nil
}

func (self *TimeDumper) Dump() *Dump {
	return self.d
}

func (self *TimeDumper) check() *Dump {
	minTime := uint64(0)
	minIndex := 0
	var ret *Dump

	for i, s := range self.dumpers {
		if s == nil {
			continue
		}

		d := s.Dump()
		if d == nil {
			continue
		}
		if ret == nil || d.Time < minTime {
			minIndex = i
			minTime = d.Time
			ret = d
		}
	}

	if ret != nil {
		self.update(minIndex)
	}
	return ret
}
