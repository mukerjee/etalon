package main

import (
	"fmt"
)

type Stat struct {
	index  int
	source string
	cur    uint64
	avg    uint64
}

var statReport = make(chan *Stat, 20)

func NewStat(source string, index int, cur, avg uint64) *Stat {
	ret := new(Stat)
	ret.index = index
	ret.source = source
	ret.cur = cur
	ret.avg = avg

	return ret
}

func bandwStr(i uint64) string {
	bits := i * 8
	if i == 0 {
		return "-"
	}
	if bits > 1e9 {
		return fmt.Sprintf("%.2fGb/s", float64(bits)/1e9)
	}
	if bits > 1e6 {
		return fmt.Sprintf("%.2fMb/s", float64(bits)/1e6)
	}
	if bits > 1e3 {
		return fmt.Sprintf("%.2fKb/s", float64(bits)/1e3)
	}
	return fmt.Sprintf("%db/s", bits)
}

func (self *Stat) String() string {
	return fmt.Sprintf("[%s] %s %s",
		self.source,
		bandwStr(self.cur),
		bandwStr(self.avg),
	)
}
