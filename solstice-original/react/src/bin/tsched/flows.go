package main

import (
	. "react/sim/config"
	"react/sim/drainer"
	. "react/sim/structs"

	"encoding/json"
	"math/rand"
)

type Flows struct {
	Nflow float64
	Noise uint64
	Max   uint64

	rand *rand.Rand
}

var _ DemandMaker = new(Flows)

func NewFlows() *Flows {
	ret := new(Flows)
	ret.Nflow = 1.0
	ret.Noise = PackBw / 2
	ret.Max = LinkBw - PackBw

	return ret
}

func ParseFlows(dec *json.Decoder) (*Flows, error) {
	ret := NewFlows()
	e := dec.Decode(ret)
	return ret, e
}

func randLane(r *rand.Rand) int {
	if Nhost <= 1 {
		panic("bug")
	}

	row := r.Intn(Nhost)
	col := (row + 1 + r.Intn(Nhost-1)) % Nhost
	return row*Nhost + col
}

func (self *Flows) randLane() int {
	return randLane(self.rand)
}

func (self *Flows) makeFlows() (budget, count Matrix) {
	count = NewMatrix()
	budget = NewMatrix()

	nflow := int(self.Nflow * float64(Nhost))

	for i := 0; i < nflow; i++ {
		lane := self.randLane()
		count[lane]++
		budget[lane] = self.Max
	}

	return
}

func (self *Flows) _make(norm uint64) Matrix {
	cross := drainer.NewCrossLimits(self.Max)
	d := drainer.NewDrainer(cross.Limits)
	budget, count := self.makeFlows()
	return d.WeightDrain(budget, count).Clone()
}

func (self *Flows) Make(norm uint64, r *rand.Rand) Matrix {
	self.rand = r
	ret := self._make(norm)
	addNoiseBudget(ret, r, self.Noise*uint64(Nhost))
	normDown(ret, norm)

	return ret
}
