package bvn

import (
	"math/rand"
	. "react/sim/config"
	. "react/sim/structs"
	"time"
)

type RandSlicer struct {
	Rand *rand.Rand
}

func timeRand() *rand.Rand {
	return rand.New(rand.NewSource(time.Now().UnixNano()))
}

func NewRandSlicer() *RandSlicer {
	ret := new(RandSlicer)
	ret.Rand = timeRand()

	return ret
}

func (self *RandSlicer) PerfectSlice(_ Sparse, pm PortMap) int {
	pm.Rand(self.Rand)
	return Nhost
}

func (self *RandSlicer) Slice(m Sparse, pm PortMap) int {
	self.PerfectSlice(m, pm)

	ret := 0
	for src, dest := range pm {
		if m[dest][src] > 0 {
			pm[src] = dest
			ret++
		} else {
			pm[src] = InvalidPort
		}
	}

	return ret
}

func (self *RandSlicer) Reset() {
	self.Rand = rand.New(rand.NewSource(0))
}
