package main

import (
	"fmt"
	"time"
)

type Reporter struct {
	stats     []*Stat
	needClear bool
}

const CSI = "\x1b["

func NewReporter(nslot int) *Reporter {
	ret := new(Reporter)
	ret.stats = make([]*Stat, nslot)

	return ret
}

func (self *Reporter) Update(s *Stat) {
	self.stats[s.index] = s
	self.Print()
}

func (self *Reporter) Print() {
	if self.needClear {
		fmt.Print(CSI, len(self.stats)+1, "A")
	}

	cur := uint64(0)
	avg := uint64(0)

	for i := 0; i < len(self.stats); i++ {
		if self.stats[i] == nil {
			fmt.Print("-")
		} else {
			stat := self.stats[i]
			fmt.Print(stat)
			cur += stat.cur
			avg += stat.avg
		}
		fmt.Print(CSI, "K\n") // clear line
	}

	fmt.Printf("[%s] %s %s", "total", bandwStr(cur), bandwStr(avg))
	fmt.Print(CSI, "K\n")

	self.needClear = true
}

func (self *Reporter) Serve() {
	for running() {
		if len(statReport) == 0 {
			time.Sleep(100 * time.Millisecond)
			continue
		}

		self.Update(<-statReport)
	}
}
