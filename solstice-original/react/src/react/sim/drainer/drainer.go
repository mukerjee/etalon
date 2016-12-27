package drainer

import (
	"math"
	. "react/sim/config"
	"react/sim/queues"
	. "react/sim/structs"
)

type Drainer struct {
	Limits []*Limit

	isDead Matrix
	used   Matrix
}

var _ queues.Puller = new(Drainer)

func NewDrainer(limits []*Limit) *Drainer {
	ret := new(Drainer)
	ret.used = NewMatrix()
	ret.isDead = NewMatrix()
	ret.Limits = limits
	return ret
}

func NewBandwDrainer(bandw uint64) *Drainer {
	limits := make([]*Limit, 0, Nhost*2)
	return NewDrainer(BandwLimits(limits, bandw))
}

func NewFreeDrainer() *Drainer {
	return NewDrainer(make([]*Limit, 0))
}

func (self *Drainer) Pull(budget Matrix) Matrix {
	return self.Drain(budget)
}

func (self *Drainer) Drain(budget Matrix) Matrix {
	return self.WeightDrain(budget, nil)
}

func (self *Drainer) updateState(budget, weight Matrix) {
	for i, b := range budget {
		if self.isDead[i] > 0 {
			continue
		}

		w := uint64(1)
		if weight != nil {
			w = weight[i]
			if w == 0 {
				self.isDead[i] = 1
				continue
			}
		}

		if self.used[i]+w > b {
			self.isDead[i] = 1
			continue
		}
	}
}

func (self *Drainer) allDead(budget, weight Matrix) bool {
	self.updateState(budget, weight)

	for _, limit := range self.Limits {
		if limit.reached {
			continue
		}
		limit.updateUsed(self.used)
		limit.updateState(self.isDead)
	}

	// logln(budget)
	// logln(self.isDead)

	for i := range budget {
		if self.isDead[i] == 0 {
			return false
		}
	}
	return true
}

func (self *Drainer) getDelta(budget, weight Matrix) uint64 {
	delta := uint64(math.MaxUint64)

	for _, limit := range self.Limits {
		if limit.reached {
			continue
		}

		aliveCount := limit.aliveCount(self.isDead)
		if aliveCount == 0 {
			// this is safe because weight will be always updated
			// on individual elements since at least one must be alive
			continue
		}

		w := limit.weight
		if weight == nil {
			w = aliveCount // will possible achive better result
		}

		delta = limit.updateDelta(delta, w)
	}

	for i, b := range budget {
		if self.isDead[i] > 0 {
			continue
		}
		w := uint64(1)
		if weight != nil {
			w = weight[i]
		}

		assert(w > 0)
		assert(b >= self.used[i]+w)
		newDelta := (b - self.used[i]) / w
		if newDelta < delta {
			delta = newDelta
		}
	}

	return delta
}

func (self *Drainer) applyDelta(delta uint64, weight Matrix) {
	for i, dead := range self.isDead {
		if dead > 0 {
			continue
		}

		if weight == nil {
			self.used[i] += delta
		} else {
			self.used[i] += weight[i] * delta
		}
	}
}

func (self *Drainer) init(budget, weight Matrix) {
	self.isDead.Clear()

	for _, limit := range self.Limits {
		limit.init(budget, weight)
	}
}

func (self *Drainer) WeightDrain(budget, weight Matrix) Matrix {
	self.used.Clear()
	self.init(budget, weight)

	for !self.allDead(budget, weight) {
		delta := self.getDelta(budget, weight)
		assert(delta > 0)
		self.applyDelta(delta, weight)
	}

	return self.used
}

func (self *Drainer) FlowRate(budget, weight Matrix) Matrix {
	ret := self.WeightDrain(budget, weight)
	ret.Mdiv(weight)

	return ret
}
