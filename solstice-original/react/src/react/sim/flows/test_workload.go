package flows

import (
	"react/sim/clock"
	. "react/sim/structs"
)

type TestWorkload struct {
}

var _ FlowWorkload = new(TestWorkload)

func NewTestWorkload() *TestWorkload {
	ret := new(TestWorkload)
	return ret
}

func (self *TestWorkload) Tick(driver *FlowDriver) {
	t := clock.T

	switch t {
	case 0:
		driver.AddBackgroundFlow(0, uint64(1e9))
	case 200:
		driver.AddBackgroundFlow(3, 3000)
	case 500:
		driver.AddQueryFlow(1, 2000, NewVectorOf(2000))
	}
}
