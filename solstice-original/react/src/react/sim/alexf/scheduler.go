package alexf

import (
	"react/sim"
	. "react/sim/config"
	. "react/sim/structs"
)

type Scheduler struct {
	DayLens      []uint64
	ClearOnSlice bool // if set to zero on picking in a permutation
	SliceFunc    func(Matrix) Matrix

	weekLen uint64
}

func NewScheduler() *Scheduler {
	ret := new(Scheduler)
	ret.SliceFunc = Slice1
	ret.DayLens = []uint64{300, 200, 100}
	for _, dayLen := range ret.DayLens {
		ret.weekLen += dayLen
	}

	return ret
}

var _ sim.Scheduler = new(Scheduler)

// Implementing the scheduler interface
// r - residue (current queue length)
// d - demand of the last time period, in number of bytes
// w - length of the last time period, normally config.WeekLen
// []*Day - the days
// Matrix - the bandwidth allocation for circuits
func (s *Scheduler) Schedule(r, d Matrix, w uint64) ([]*Day, Matrix) {
	// r is ignored

	assert(w == WeekLen)
	// assert(WeekLen%s.DayLen == 0)

	// so, you really mean the residue (current queue length)
	dem := r.Clone() // get a cloned buffer
	// nday := WeekLen / s.DayLen
	// assert(s.DayLen > NightLen)
	assert(WeekLen == s.weekLen)

	var ret []*Day
	bandw := NewMatrix()

	for _, dayLen := range s.DayLens {
		slice := s.SliceFunc(dem)

		pm := slice.ToPortMap()
		s.remove(dem, pm, dayLen)

		clearDiag(slice)

		day := &Day{Sched: slice, Len: dayLen}
		ret = append(ret, day)

		bandw.Madd(slice)
	}

	bandw.Binary()
	bandw.Mul(LinkBw - PackBw)

	return ret, bandw
}

func clearDiag(m Matrix) {
	for i := 0; i < Nhost; i++ {
		m[Lane(i, i)] = 0
	}
}

func (s *Scheduler) remove(d Matrix, pm PortMap, dayLen uint64) {
	if s.ClearOnSlice {
		for row, col := range pm {
			d[Lane(row, col)] = 0
		}
	} else {
		delta := dayLen * LinkBw
		for row, col := range pm {
			lane := Lane(row, col)
			n := d[lane]
			if n > delta {
				d[lane] -= n
			} else {
				d[lane] = 0
			}
		}
	}
}
