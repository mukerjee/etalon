package bvn

import (
	"fmt"
	. "react/sim/structs"
)

type Slice struct {
	PortMap PortMap
	Weight  uint64
	Id      int // for split debug tracking only
}

func NewSlice(id int) *Slice {
	ret := new(Slice)
	ret.PortMap = NewPortMap()
	ret.Id = id

	return ret
}

func EmptySlice(w uint64) *Slice {
	ret := NewSlice(0)
	ret.Weight = w

	return ret
}

func (self *Slice) split(w uint64) *Slice {
	assert(w <= self.Weight)

	ret := new(Slice)
	ret.PortMap = self.PortMap
	ret.Weight = w
	self.Weight -= w
	ret.Id = self.Id

	return ret
}

func (self *Slice) String() string {
	return fmt.Sprintf("%s x%d #%d", self.PortMap, self.Weight, self.Id)
}

func slicesToDays(ss []*Slice) []*Day {
	ret := make([]*Day, len(ss))
	for i, s := range ss {
		ret[i] = NewDay(s.PortMap.Matrix(), s.Weight)
	}

	return ret
}
