package diag

import (
	"sort"

	"react/conf"
	"react/sim"
	. "react/sim/config"
	. "react/sim/structs"
)

type Scheduler struct {
	demand   Matrix
	sum      Vector
	bandwBuf Matrix

	PureCircuit  bool
	SafeBandw    bool
	FullBandw    bool
	SchedResidue bool
	CleanOnIdle  bool
}

type diagDay struct {
	id      int
	nbyte   uint64
	weight  uint64
	weight_ uint64
}

type diagDays []*diagDay
type diagById struct{ diagDays }
type diagByWeight struct{ diagDays }

func (dd diagDays) Len() int      { return len(dd) }
func (dd diagDays) Swap(i, j int) { dd[i], dd[j] = dd[j], dd[i] }
func (dd diagById) Less(i, j int) bool {
	return dd.diagDays[i].id < dd.diagDays[j].id
}
func (dd diagByWeight) Less(i, j int) bool {
	return dd.diagDays[i].weight < dd.diagDays[j].weight
}

var _ sort.Interface = new(diagById)
var _ sort.Interface = new(diagByWeight)

var _ sim.Scheduler = new(Scheduler)

func NewScheduler() *Scheduler {
	ret := new(Scheduler)
	ret.demand = NewMatrix()
	ret.sum = NewVector()
	ret.bandwBuf = NewMatrix()
	ret.FullBandw = conf.Conf.Sched.FullBandw

	return ret
}

func permMatrix(i int) Matrix {
	m := NewMatrix()
	for j := 0; j < Nhost; j++ {
		m[Lane(j, (j+i)%Nhost)] = 1
	}

	return m
}

func (self *Scheduler) minDayLen() uint64 {
	ret := MinDayLen
	if ret < NightLen {
		ret = NightLen + 1
	}
	return ret
}

func (self *Scheduler) applyWeights(days []*diagDay) {
	for _, d := range days {
		d.weight = d.weight_
	}
}

func (self *Scheduler) scale(days []*diagDay) []*diagDay {
	if !self.PureCircuit {
		sort.Sort(diagByWeight{days})

		maxSplit := -1
		maxServed := uint64(0)

		for i := range days {
			packDays := days[:i]
			circDays := days[i:]
			self.scale_(circDays)
			served := self.eval_(circDays, self.sumBytes(packDays))
			// println(i, served)
			if maxSplit < 0 || served > maxServed {
				maxSplit = i
				maxServed = served
			}
		}
		// println(maxSplit, maxServed)

		days = days[maxSplit:]
		sort.Sort(diagById{days})
	}

	self.scale_(days)
	self.applyWeights(days)

	return days
}

func (self *Scheduler) sumBytes(days []*diagDay) uint64 {
	ret := uint64(0)
	for _, d := range days {
		ret += d.nbyte / uint64(Nhost)
	}
	return ret
}

func (self *Scheduler) eval_(days []*diagDay, packDemand uint64) uint64 {
	circServed := uint64(0)
	packServed := uint64(0)
	for _, d := range days {
		// println("d:", d.weight, d.weight_)
		nbyte := d.nbyte / uint64(Nhost)
		assert(d.weight_ > NightLen)
		t := d.weight_ - NightLen

		circBw := nbyte / t
		if nbyte%t != 0 {
			circBw++
		}
		if circBw > LinkBw {
			circBw = LinkBw
		}
		_circServed := circBw * t
		if _circServed > nbyte {
			_circServed = nbyte
		}

		circServed += _circServed

		packBw := LinkBw - circBw
		// println("packbw:", packBw)
		if packBw > PackBw {
			packBw = PackBw
		}
		packServed += NightLen*PackBw + t*packBw
	}

	if packServed > packDemand {
		packServed = packDemand
	}

	// println(circServed, packServed, circServed + packServed)

	return circServed + packServed
}

func (self *Scheduler) scale_(days []*diagDay) {
	minDayLen := self.minDayLen()

	assert(WeekLen >= minDayLen*uint64(len(days)))

	scaleTo := WeekLen - minDayLen*uint64(len(days))
	if scaleTo == 0 {
		for _, day := range days {
			day.weight_ = minDayLen
		}
		return
	}

	total := uint64(0)
	for _, day := range days {
		day.weight_ = day.weight + NightLen
		if day.weight_ < minDayLen {
			day.weight_ = 0
		} else {
			day.weight_ -= minDayLen
		}
		total += day.weight_
	}

	if total > 0 {
		total2 := uint64(0)
		for _, day := range days {
			day.weight_ = day.weight_ * scaleTo / total
			total2 += day.weight_
		}

		i := 0
		for total2 > scaleTo {
			days[i].weight_--
			i = (i + 1) % len(days)
			total2--
		}

		for total2 < scaleTo {
			days[i].weight_++
			i = (i + 1) % len(days)
			total2++
		}

		assert(total2 == scaleTo)
	} else {
		assert(len(days) > 0)
		nday := uint64(len(days))
		total2 := uint64(0)
		for i, day := range days {
			day.weight_ = scaleTo / nday
			if i < int(scaleTo%nday) {
				day.weight_++
			}
			total2 += day.weight_
		}
		assert(total2 == scaleTo)
	}

	for _, day := range days {
		day.weight_ += minDayLen
	}
}

func (self *Scheduler) bandw(days []*Day, bytes Matrix) Matrix {
	buf := self.bandwBuf
	buf.Clear()

	for _, day := range days {
		for j := 0; j < Nlane; j++ {
			assert(day.Len > NightLen)
			buf[j] += day.Sched[j] * (day.Len - NightLen)
		}
	}

	for i := 0; i < Nlane; i++ {
		ntick := buf[i]
		if ntick > 0 {
			buf[i] = bytes[i] / ntick
			if bytes[i]%ntick != 0 {
				buf[i]++
			}
		}
	}

	if self.SafeBandw {
		buf.AtMost(LinkBw - PackBw)
	} else if self.FullBandw {
		buf.NonZeroAtLeast(LinkBw)
		buf.AtMost(LinkBw)
	} else {
		buf.AtMost(LinkBw)
	}

	buf.NonZeroAtLeast(LinkBw - PackBw)

	return buf
}

func (self *Scheduler) sumUp() {
	self.sum.Clear()

	for i := 1; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			row := j
			col := (j + i) % Nhost
			lane := Lane(row, col)
			self.sum[i] += self.demand[lane]
		}
	}

	// self.sum.Div(uint64(Nhost) * LinkBw)
}

func (self *Scheduler) buildDays() []*diagDay {
	days := make([]*diagDay, 0, Nhost-1)
	for i := 1; i < Nhost; i++ {
		if self.sum[i] == 0 {
			continue
		}

		s := self.sum[i] / (uint64(Nhost) * LinkBw)
		if self.sum[i]%(uint64(Nhost)*LinkBw) != 0 {
			s++
		}
		/*
			if !self.PureCircuit {
				if s <= NightLen {
					continue
				}
				if MinDayLen > NightLen && s <= MinDayLen-NightLen {
					continue
				}
			}
		*/ // we will trim later, so no need to trim here

		day := &diagDay{i, self.sum[i], s, 0}
		days = append(days, day)
	}

	return days
}

func (self *Scheduler) convert(ddays []*diagDay) []*Day {
	ret := make([]*Day, len(ddays))
	for i, dd := range ddays {
		ret[i] = NewDay(permMatrix(dd.id), dd.weight)
	}
	return ret
}

func (self *Scheduler) cleaningDays() ([]*Day, Matrix) {
	r := WeekLen % uint64(Nhost-1)
	dayLen := WeekLen / uint64(Nhost-1)
	assert(dayLen >= MinDayLen)
	bandw := self.bandwBuf
	bandw.Clear()

	ret := make([]*Day, 0, Nhost-1)
	for i := 1; i < Nhost; i++ {
		p := permMatrix(i)
		day := &Day{p, dayLen}
		if i <= int(r) {
			day.Len++
		}
		ret = append(ret, day)
		bandw.Madd(p)
	}
	bandw.Mul(LinkBw - PackBw)
	return ret, bandw
}

func (self *Scheduler) Schedule(r, d Matrix, w uint64) ([]*Day, Matrix) {
	if r != nil && self.SchedResidue {
		copy(self.demand, r)
	} else {
		self.demand.Clear()
	}

	// println(d.MatrixStr())
	self.demand.Madd(d)
	self.sumUp()

	ddays := self.buildDays()
	if len(ddays) == 0 {
		if self.CleanOnIdle {
			return self.cleaningDays()
		}
		return []*Day{{NewMatrix(), WeekLen}}, NewMatrix()
	}

	ddays = self.scale(ddays)
	/*
		println(self.sum.String())
		for i, day := range ddays {
			println(i, day.weight)
		}
	*/

	days := self.convert(ddays)
	bandw := self.bandw(days, self.demand)
	// println(bandw.String())

	return days, bandw
}
