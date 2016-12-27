package drainer

import (
	"math"
	. "react/sim/config"
	. "react/sim/structs"
)

type LinkDrainer struct {
	Bounds   []uint64
	isUplink bool
	used     Matrix
}

func NewLinkDrainer(bound uint64, isUplink bool) *LinkDrainer {
	ret := new(LinkDrainer)
	ret.Bounds = make([]uint64, Nhost)
	for i := 0; i < Nhost; i++ {
		ret.Bounds[i] = bound
	}
	ret.isUplink = isUplink
	ret.used = NewMatrix()

	return ret
}

func NewUplinkDrainer(bound uint64) *LinkDrainer {
	return NewLinkDrainer(bound, true)
}

func NewDownlinkDrainer(bound uint64) *LinkDrainer {
	return NewLinkDrainer(bound, false)
}

func (self *LinkDrainer) Pull(budget Matrix) Matrix {
	self.used.Clear()

	for i := 0; i < Nhost; i++ {
		self.drain(i, budget)
	}

	return self.used
}

func (self *LinkDrainer) lane(link, i int) int {
	if self.isUplink {
		return link*Nhost + i
	}
	return i*Nhost + link
}

func (self *LinkDrainer) drain(link int, budget Matrix) {
	used := uint64(0)
	bound := self.Bounds[link]

	for used < bound {
		nalive := uint64(0)
		delta := uint64(math.MaxUint64)
		for i := 0; i < Nhost; i++ {
			lane := self.lane(link, i)
			if self.used[lane] < budget[lane] {
				nalive++
				newDelta := budget[lane] - self.used[lane]
				if newDelta < delta {
					delta = newDelta
				}
			}
		}

		if nalive == 0 {
			break
		}

		newDelta := (bound - used) / nalive
		if newDelta == 0 {
			break
		}

		if newDelta < delta {
			delta = newDelta
		}

		for i := 0; i < Nhost; i++ {
			lane := self.lane(link, i)
			if self.used[lane] < budget[lane] {
				self.used[lane] += delta
				assert(self.used[lane] <= budget[lane])
				used += delta
			}
		}
		assert(used <= bound)
	}
}
