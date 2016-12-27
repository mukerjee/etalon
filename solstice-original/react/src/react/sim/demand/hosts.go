// Package dem takes a demand file and convert it to a number of hosts
package demand

import (
	"errors"

	dem "react/demand"
	"react/sim"
	. "react/sim/blocks"
	. "react/sim/config"
	"react/sim/packet"
	. "react/sim/structs"
	. "react/sim/win"
)

// Hosts runs through a number of fixed demand
// matrices (periods), it also serves as a demand estimator
// for that the future is predictable
type Hosts struct {
	window    *Window // the window for demand estimating
	demandEst Matrix
	zeros     Matrix
	demand    *dem.Demand
	period    *dem.Period
	cur       Matrix

	tperiod uint64
	index   int // the index of current stage
}

var _ sim.Hosts = new(Hosts)
var _ sim.Estimator = new(Hosts)

func NewHosts(d *dem.Demand) (ret *Hosts) {
	ret = new(Hosts)

	ret.demandEst = NewMatrix()
	ret.zeros = NewMatrix()
	ret.window = NewWindow()
	ret.demand = d

	ret.Restart()
	return ret
}

func DemandHosts(path string) (*Hosts, error) {
	d, e := dem.Load(path)
	if e != nil {
		return nil, e
	}
	if d.Nhost() != Nhost {
		return nil, errors.New("nhost mismatch")
	}

	return NewHosts(d), nil
}

func periodMatrix(p *dem.Period) Matrix {
	assert(Nhost == len(p.D))
	ret := NewMatrix()

	for i, v := range p.D {
		assert(Nhost == len(v))
		for j, r := range v {
			ret[i*Nhost+j] = r
		}
	}

	return ret
}

func (self *Hosts) Restart() {
	self.tperiod = 0
	self.index = 0
	self.window.Clear()

	if self.demand != nil && self.demand.Nperiod() > 0 {
		self.period = self.demand.Period(0)
		self.cur = periodMatrix(self.period)
	}

	assert(WeekLen > 0)
	for i := uint64(0); i < WeekLen-1; i++ {
		self.tick()
	}
}

// increase the time iterator by one tick
// returns the last demand matrix
func (self *Hosts) tick() (ret Matrix) {
	if self.window.Len() < int(WeekLen) {
		self.window.Enq(self.cur)
	} else {
		self.window.Shift(self.cur)
	}

	assert(self.window.Len() <= int(WeekLen))
	if self.window.Len() == int(WeekLen) {
		ret = self.window.Front() // and we are ready
	}

	if self.period != nil {
		self.tperiod++

		if self.tperiod == self.period.N {
			self.tperiod = 0
			self.index++
			assert(self.index <= self.demand.Nperiod())

			if self.index == self.demand.Nperiod() {
				self.period = nil
				self.cur = self.zeros
			} else {
				self.period = self.demand.Period(self.index)
				self.cur = periodMatrix(self.period)
			}
		}
	}

	return ret
}

func (self *Hosts) Tick(sender Sender) {
	m := self.tick()
	assert(m != nil)

	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			lane := i*Nhost + j
			size := m[lane]
			if size > 0 {
				sender.Send(packet.NewPacket(size, lane))
			}
		}
	}
}

func (self *Hosts) Estimate() (Matrix, uint64) {
	copy(self.demandEst, self.window.Sum())
	return self.demandEst, WeekLen
}

func (self *Hosts) Push(_ *packet.Packet) {
	// passive one-way hosts, do nothing on recv
}

func (self *Hosts) Len() uint64 {
	return self.demand.Len()
}

func (self *Hosts) String() string {
	return self.demand.String()
}
