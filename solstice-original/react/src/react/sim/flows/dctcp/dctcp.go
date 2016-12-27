package dctcp

import (
	"math/rand"
	. "react/sim/config"
	. "react/sim/flows"
	. "react/sim/rand/pois"
	. "react/sim/structs"
)

type DCTCPWorkload struct {
	flowCounter  *PoisCounter
	queryCounter *PoisCounter
	rand         *rand.Rand

	NoFlows   bool
	NoQueries bool
	NoBig     bool
}

var _ FlowWorkload = new(DCTCPWorkload)

var QueryFreqScaler float64 = 1
var QuerySizeScaler uint64 = 1

var FlowSizeScaler uint64 = 1
var FlowFreqScaler float64 = 1

var SmallFlowThres uint64 = 5e4

const (
	FlowCount  = 5
	QueryCount = 16

	// The 6 here is like a golden parameter
	// to make the actural concurrent big flows
	// to be (sort of) consistent with the DCTCP
	// paper
	FlowPeriod  = 50000 * 6
	QueryPeriod = 50000 * 6
)

func NewDCTCPWorkload(seed int64) *DCTCPWorkload {
	return NewDCTCPWorkloadScaled(seed, 1)
}

func NewDCTCPWorkloadScaled(seed int64, scaler float64) *DCTCPWorkload {
	ret := new(DCTCPWorkload)
	ret.rand = rand.New(rand.NewSource(seed))
	ret.flowCounter = NewPeriodPoisson(ret.rand, FlowCount,
		int(FlowPeriod/FlowFreqScaler/float64(Nhost)/scaler),
	)
	ret.queryCounter = NewPeriodPoisson(ret.rand, QueryCount,
		int(QueryPeriod/QueryFreqScaler/float64(Nhost)/scaler),
	)

	return ret
}

var flowSizes = func() []int64 {
	floatSizes := []float64{2e3, 1e4, 3e4, 6e4, 1e5, 1e6, 7e6, 2e7}
	ret := make([]int64, 0, len(floatSizes))
	for _, f := range floatSizes {
		ret = append(ret, int64(f))
	}
	return ret
}()

func (self *DCTCPWorkload) randFlowSize() uint64 {
	assert(len(flowSizes) > 1)
	n := self.rand.Intn(len(flowSizes) - 1)
	min := flowSizes[n]
	max := flowSizes[n+1]
	assert(max > min)
	assert(min > 0)
	return uint64(min + self.rand.Int63n(max-min))
}

func randLane(r *rand.Rand) int {
	src := r.Intn(Nhost)
	dest := r.Intn(Nhost)
	for dest == src {
		dest = r.Intn(Nhost)
	}
	return src*Nhost + dest
}

func requestSize(r *rand.Rand) uint64 {
	return uint64(1.6e3)
}

func respondSize(r *rand.Rand) uint64 {
	return uint64(1.6e3) + uint64(r.Intn(0.4e3))
}

func (self *DCTCPWorkload) makeRespSizes() Vector {
	ret := NewVector()
	for i := 0; i < Nhost; i++ {
		ret[i] = respondSize(self.rand)
	}
	return ret
}

const modelScaler = 10

func (self *DCTCPWorkload) addBackgroundFlows(driver *FlowDriver) {
	if self.NoFlows {
		return
	}

	n := self.flowCounter.Tick()
	scaler := uint64(FlowFreqScaler)
	assert(scaler >= 1)

	for i := 0; i < n; i++ {
		lane := randLane(self.rand)
		size := self.randFlowSize()
		size *= FlowSizeScaler * modelScaler
		if size <= SmallFlowThres*modelScaler {
			// small flow
			driver.AddBackgroundFlow(lane, size)
		} else {
			if !self.NoBig {
				// big flow
				if self.rand.Intn(int(scaler)) == 0 {
					driver.AddBackgroundFlow(lane, size*scaler)
				}
			}
		}
	}
}

func (self *DCTCPWorkload) addQueryFlows(driver *FlowDriver) {
	if self.NoQueries {
		return
	}

	n := self.queryCounter.Tick()
	for i := 0; i < n; i++ {
		src := self.rand.Intn(Nhost)
		reqSize := requestSize(self.rand)
		respSizes := self.makeRespSizes()

		reqSize *= QuerySizeScaler * modelScaler
		for j := range respSizes {
			respSizes[j] *= QuerySizeScaler * modelScaler
		}

		driver.AddQueryFlow(src, reqSize, respSizes)
	}
}

func (self *DCTCPWorkload) Tick(driver *FlowDriver) {
	self.addBackgroundFlows(driver)
	self.addQueryFlows(driver)
}
