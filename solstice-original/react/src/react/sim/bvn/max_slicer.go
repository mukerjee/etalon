package bvn

import (
	"sort"

	. "react/sim/config"
	. "react/sim/structs"
)

type MaxSlicer struct {
	anySlicer *AnySlicer

	filtered Sparse
}

var _ Slicer = new(MaxSlicer)

func NewMaxSlicer() *MaxSlicer {
	ret := new(MaxSlicer)
	ret.anySlicer = NewAnySlicer()
	ret.filtered = NewSparse()

	return ret
}

type u64s []uint64

func (s u64s) Len() int           { return len(s) }
func (s u64s) Swap(i, j int)      { s[i], s[j] = s[j], s[i] }
func (s u64s) Less(i, j int) bool { return s[i] > s[j] }

func (self *MaxSlicer) Slice(m Sparse, pm PortMap) int {
	// only looks for perfect slicing
	ths := make([]uint64, 0, Nhost*10)
	for _, scol := range m {
		for _, th := range scol {
			ths = append(ths, th)
		}
	}

	sort.Sort(u64s(ths))
	lastTh := uint64(0)
	for _, th := range ths {
		if th == lastTh {
			continue
		}
		lastTh = th
		filtered := self.filtered.ThresFrom(m, th)
		pm.Clear()
		n := self.anySlicer.Slice(filtered, pm)
		if n == Nhost {
			return Nhost
		}
	}

	return 0
}

func (self *MaxSlicer) Reset() {
	self.anySlicer.Reset()
}
