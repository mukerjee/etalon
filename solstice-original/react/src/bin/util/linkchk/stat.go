package main

import (
	"fmt"
)

type Stat struct {
	source    string
	group     uint64
	count     uint64
	perGroup  uint64
	totalLost uint64

	index int
	ttl   int
}

func NewStat(source string, group, count, perGroup, totalLost uint64) *Stat {
	ret := new(Stat)
	ret.source = source
	ret.group = group
	ret.count = count
	ret.perGroup = perGroup
	ret.totalLost = totalLost

	ret.ttl = statTTL

	return ret
}

func (self *Stat) SetIndex(index int) {
	self.index = index
}

func (self *Stat) String() string {
	ret := fmt.Sprintf("[%s] %d: ", self.source, self.group)

	if self.count != self.perGroup {
		ret += fmt.Sprintf(" %d lost   ", self.perGroup-self.count)
	}

	if self.totalLost > 0 {
		ret += fmt.Sprintf(" total %d lost", self.totalLost)
	} else {
		ret += fmt.Sprintf(" - ")
	}

	return ret
}
