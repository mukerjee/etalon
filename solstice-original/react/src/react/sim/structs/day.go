package structs

import (
	"fmt"
	"log"

	. "react/sim/config"
)

type Day struct {
	Sched Matrix
	Len   uint64
}

func NewDay(sched Matrix, length uint64) *Day {
	ret := new(Day)
	ret.Sched = sched

	// nigth length safe guarding
	if length <= NightLen {
		length = NightLen + 1
	}

	ret.Len = length

	return ret
}

func (self *Day) String() string {
	return fmt.Sprintf("x%d %s", self.Len, self.Sched)
}

func (self *Day) ApplyBandw(bandw Matrix) {
	self.Sched.Binary()
	self.Sched.Mmul(bandw)
}

func (self *Day) Check() error {
	srcs := make([]bool, Nhost)
	dests := make([]bool, Nhost)

	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			lane := i*Nhost + j
			if self.Sched[lane] == 0 {
				continue
			}

			if i == j {
				self.Sched[lane] = 0
				log.Printf("warning: scheduling node %d send to itself", i)
			}
			if srcs[i] {
				return fmt.Errorf("multiple src %d", i)
			}
			if dests[j] {
				return fmt.Errorf("multiple dest %d", j)
			}

			srcs[i] = true
			dests[j] = true
		}
	}

	return nil
}
