package main

import (
	"time"
)

type Timings struct {
	ToNight  uint64
	ToPlan   uint64
	ToMap    uint64
	ToSync   uint64
	ToSwitch uint64
}

const nsPerCycle = 6.4

func cycles(d time.Duration) uint64 {
	return uint64(float64(d.Nanoseconds()) / nsPerCycle)
}

func us(i int) uint64 {
	return cycles((time.Duration)(i) * time.Microsecond)
}

func MindspdTimings() *Timings {
	return &Timings{
		ToNight:  us(20),
		ToPlan:   us(40),
		ToMap:    us(30),
		ToSync:   us(20),
		ToSwitch: us(10),
	}
}

func MordiaTimings() *Timings {
	return &Timings{
		ToNight:  us(30),
		ToPlan:   us(95),
		ToMap:    us(90),
		ToSync:   us(40),
		ToSwitch: us(20),
	}
}

func (self *Timings) packTo(p *Packer) {
	p.u64(self.ToNight)
	p.u64(self.ToPlan)
	p.u64(self.ToMap)
	p.u64(self.ToSync)
	p.u64(self.ToSwitch)
}

func (self *Timings) check() bool {
	// TODO: check timings
	return true
}
