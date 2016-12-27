package simple

import (
	"math/rand"

	. "react/sim/config"
	. "react/sim/flows"
	. "react/sim/rand/pois"
)

type SimpleWorkload struct {
	bigFlowCounter   *PoisCounter
	smallFlowCounter *PoisCounter
	rand             *rand.Rand

	NoBig   bool
	NoSmall bool
}

var BigFlowSizeScaler uint64 = 1
var BigFlowFreqScaler float64 = 1
var SmallFlowSizeScaler uint64 = 1
var SmallFlowFreqScaler float64 = 1

const (
	BigFlowCount   = 5
	SmallFlowCount = 100

	BigFlowPeriod   = 200000
	SmallFlowPeriod = 10000
)

func New(seed int64, scaler float64) *SimpleWorkload {
	ret := new(SimpleWorkload)
	ret.rand = rand.New(rand.NewSource(seed))
	ret.bigFlowCounter = NewPeriodPoisson(ret.rand, BigFlowCount,
		int(BigFlowPeriod/BigFlowFreqScaler/float64(Nhost)/scaler))
	ret.smallFlowCounter = NewPeriodPoisson(ret.rand, SmallFlowCount,
		int(SmallFlowPeriod/SmallFlowFreqScaler/float64(Nhost)/scaler))

	return ret
}

func randLane(r *rand.Rand) int {
	src := r.Intn(Nhost)
	dest := r.Intn(Nhost)
	for dest == src {
		dest = r.Intn(Nhost)
	}
	return src*Nhost + dest
}

func (self *SimpleWorkload) Tick(driver *FlowDriver) {
	n := self.bigFlowCounter.Tick()
	if !self.NoBig {
		scaler := uint64(BigFlowFreqScaler)
		if scaler == 0 {
			panic("bug")
		}

		for i := 0; i < n; i++ {
			lane := randLane(self.rand)
			size := 10000000 * BigFlowSizeScaler
			if self.rand.Intn(int(scaler)) == 0 {
				driver.AddElephantFlow(lane, size*scaler)
			}
		}
	}

	n = self.smallFlowCounter.Tick()
	if !self.NoSmall {
		for i := 0; i < n; i++ {
			lane := randLane(self.rand)
			size := 3000 * SmallFlowSizeScaler
			driver.AddMouseFlow(lane, size)
		}
	}
}
