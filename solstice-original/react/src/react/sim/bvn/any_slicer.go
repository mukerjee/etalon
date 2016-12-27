package bvn

import (
	. "react/conf"
	. "react/sim/config"
	. "react/sim/structs"

	"math/rand"
	"sort"
)

type AnySlicer struct {
	EdgesLooked int
	SkipPrefind bool

	portMap  PortMap
	searched []bool
	matched  []bool
	rand     *rand.Rand
}

var _ Slicer = new(AnySlicer)

func NewAnySlicer() *AnySlicer {
	ret := new(AnySlicer)
	ret.searched = make([]bool, Nhost)
	ret.matched = make([]bool, Nhost)
	ret.SkipPrefind = Conf.Sched.SkipPrefind
	ret.Reset()
	return ret
}

func (self *AnySlicer) clear() {
	for i := 0; i < Nhost; i++ {
		self.searched[i] = false
	}
}

func (self *AnySlicer) prefindFor(data Sparse, dest int) bool {
	rows := make([]int, 0, 20)
	for i := range data[dest] {
		rows = append(rows, i)
	}
	sort.Ints(rows)
	perm := self.rand.Perm(len(rows))

	for _, p := range perm {
		i := rows[p]

		d := self.portMap[i]
		if d == InvalidPort {
			self.portMap[i] = dest
			return true
		}
	}

	return false
}

func (self *AnySlicer) findFor(data Sparse, dest int) bool {
	rows := make([]int, 0, 20)
	for i, a := range data[dest] {
		assert(a > 0)
		rows = append(rows, i)
	}
	sort.Ints(rows) // to make it deterministic

	if len(rows) == 0 {
		return false
	}

	perm := self.rand.Perm(len(rows))

	for _, p := range perm {
		self.EdgesLooked++
		i := rows[p]

		// already searched, skip
		if self.searched[i] {
			continue
		}

		self.searched[i] = true

		d := self.portMap[i]
		assert(d == InvalidPort || (d >= 0 && d < Nhost))
		if d == InvalidPort || self.findFor(data, d) {
			self.portMap[i] = dest
			return true
		}
	}

	return false
}

func (self *AnySlicer) Reset() {
	self.rand = rand.New(rand.NewSource(3))
}

func (self *AnySlicer) Slice(m Sparse, pm PortMap) int {
	// self.EdgesLooked = 0
	self.portMap = pm

	nmatch := 0

	if !self.SkipPrefind {
		for i := 0; i < Nhost; i++ {
			self.matched[i] = false
			if self.prefindFor(m, i) {
				nmatch++
				self.matched[i] = true
			}
		}
	}

	for i := 0; i < Nhost; i++ {
		if self.matched[i] {
			continue
		}

		self.clear()
		if self.findFor(m, i) {
			nmatch++
		}
	}

	return nmatch
}
