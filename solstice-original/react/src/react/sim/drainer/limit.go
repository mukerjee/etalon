package drainer

import (
	"fmt"
	. "react/sim/config"
	. "react/sim/structs"
)

type Limit struct {
	Lanes []int
	Bound uint64

	used    uint64
	weight  uint64
	reached bool
}

func (self *Limit) String() string {
	return fmt.Sprintf("[%d <=%d]", self.Lanes, self.Bound)
}

func NewLimit(lanes []int, bound uint64) *Limit {
	ret := new(Limit)
	ret.Lanes = lanes
	ret.Bound = bound
	return ret
}

func RowLimit(row int, bound uint64) *Limit {
	lanes := make([]int, Nhost)
	for i := range lanes {
		lanes[i] = row*Nhost + i
	}

	return NewLimit(lanes, bound)
}

func ColLimit(col int, bound uint64) *Limit {
	lanes := make([]int, Nhost)
	for i := range lanes {
		lanes[i] = i*Nhost + col
	}

	return NewLimit(lanes, bound)
}

func (self *Limit) clear() {
	self.used = 0
	self.reached = false
}

func (self *Limit) countWeight(budget, weight Matrix) {
	self.weight = 0
	for _, lane := range self.Lanes {
		if budget[lane] == 0 {
			continue
		}

		if weight == nil {
			self.weight++
		} else {
			self.weight += weight[lane]
		}
	}
}

func (self *Limit) init(budget, weight Matrix) {
	self.clear()
	self.countWeight(budget, weight)
}

func (self *Limit) updateUsed(used Matrix) {
	self.used = uint64(0)
	for _, lane := range self.Lanes {
		self.used += used[lane]
	}
}

func (self *Limit) closeUp(isDead Matrix) {
	self.reached = true
	for _, lane := range self.Lanes {
		isDead[lane] = 1
	}
}

func (self *Limit) updateState(isDead Matrix) {
	if self.weight == 0 {
		self.closeUp(isDead)
	} else if self.used+self.weight > self.Bound {
		self.closeUp(isDead)
	}
}

func (self *Limit) updateDelta(delta, w uint64) uint64 {
	assert(self.Bound >= self.used+w)
	newDelta := (self.Bound - self.used) / w
	if newDelta < delta {
		return newDelta
	}
	return delta
}

func (self *Limit) aliveCount(isDead Matrix) uint64 {
	ret := uint64(0)

	for _, lane := range self.Lanes {
		if isDead[lane] > 0 {
			continue
		}
		ret++
	}

	return ret
}
