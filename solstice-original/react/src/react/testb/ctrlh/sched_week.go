package ctrlh

import (
	"bytes"

	"react/conf"
	"react/testb/setup"
)

type SchedWeek struct {
	Index int
	Days  []*SchedDay
}

func newWeek(index int) *SchedWeek {
	ret := new(SchedWeek)
	ret.Index = index
	ret.Days = make([]*SchedDay, 0, setup.Nhost*2)
	return ret
}

const DefaultWeekDanLen = 214

func DefaultWeek(index int) *SchedWeek {
	hosts := conf.Conf.Hosts
	n := len(hosts)

	ret := &SchedWeek{
		Index: index,
		Days:  make([]*SchedDay, n-1),
	}

	for step := 1; step < n; step++ {
		ind := step - 1

		day := &SchedDay{
			Index: ind,
			Len:   DefaultWeekDanLen,
			Lanes: make([]*SchedLane, n),
		}
		ret.Days[ind] = day

		for i := 0; i < n; i++ {
			day.Lanes[i] = &SchedLane{
				Src:  hosts[i],
				Dest: hosts[(i+step)%n],
				Rate: 98,
				// Rate: setup.CircRatio,
			}
		}
	}

	return ret
}

func (self *SchedWeek) WriteTo(buf *bytes.Buffer) {
	for _, day := range self.Days {
		day.WriteTo(buf)
	}
}

func (self *SchedWeek) Len() uint64 {
	ret := uint64(0)
	for _, day := range self.Days {
		ret += day.Len
	}

	return ret
}

func (self *SchedWeek) MinRate() int {
	// TODO: need a better solution
	ret := 100
	for _, d := range self.Days {
		nlane := len(d.Lanes)
		if nlane > setup.Nhost {
			panic("too many lanes")
		} else if nlane == setup.Nhost {
			for _, lane := range d.Lanes {
				if lane.Rate < ret {
					ret = lane.Rate
				}
			}
		} else {
			ret = setup.CircRatio
		}
	}

	if ret < 0 || ret > 100 {
		panic("invalid rate")
	}

	return ret
}
