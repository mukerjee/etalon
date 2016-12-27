package react

import "time"

type Slower struct {
	period        time.Duration
	t             uint64
	tickPerPeriod uint64
	tick          <-chan time.Time
}

func NewSlower(tickPerPeriod uint64, period time.Duration) *Slower {
	ret := new(Slower)
	ret.period = period
	ret.tickPerPeriod = tickPerPeriod
	ret.t = 0
	ret.start()

	return ret
}

func (self *Slower) Yield() {
	if self.tickPerPeriod == 0 {
		return
	}

	t := Time() // get the logic time

	for self.t < t {
		// need some slowing

		<-self.tick // wait for tick
		self.t += self.tickPerPeriod
		self.start()
	}
}

func (self *Slower) start() {
	self.tick = time.After(self.period)
}
