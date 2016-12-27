package bvn

import (
	// "log"
	// "fmt"
	// "react/sim/clock"

	"math/rand"

	. "react/conf"
	"react/sim"
	. "react/sim/config"
	. "react/sim/structs"
)

type Scheduler struct {
	Name string

	interleaver *Interleaver

	demand Matrix
	nbyte  Matrix
	circs  Matrix
	ntick  Matrix

	rowBuf Vector
	colBuf Vector

	NoSaveTarget bool
	SafeBandw    bool
	SkipBandw    bool
	DoTrim       bool
	DoAlign      bool
	StarveAlign  bool // if we are allowed to starve an element on aligning
	DoScale      bool
	DoAlexScale  bool
	DoMerge      bool
	DoShuffle    bool
	DoDiscard    bool
	DoInterleave bool
	DoStuff      bool
	FullStuff    bool
	DoGreedy     bool
	SchedResidue bool
	*DecompOpts

	Stuffer Stuffer
	Grid    uint64

	decomposer *Decomposer
	random     *rand.Rand

	Target  Matrix   // the matrix that is decomposed, in unit of ticks
	DayLens []uint64 // original day lengths after decomposed
}

var _ sim.Scheduler = new(Scheduler)

func NewScheduler() *Scheduler {
	ret := new(Scheduler)
	ret.interleaver = NewInterleaver()

	ret.demand = NewMatrix()
	ret.nbyte = NewMatrix()
	ret.circs = NewMatrix()
	ret.ntick = NewMatrix()

	ret.rowBuf = NewVector()
	ret.colBuf = NewVector()

	ret.SafeBandw = Conf.Sched.SafeBandw
	ret.DoInterleave = Conf.Sched.Interleave
	ret.DoTrim = (PackBw > 0)
	ret.DoAlign = false
	ret.DoScale = true
	ret.DoAlexScale = true
	ret.DoMerge = true
	ret.DoStuff = true
	ret.DoGreedy = true
	ret.DoShuffle = Conf.Sched.Shuffle
	ret.Stuffer = NewSharpStuffer()

	ret.DecompOpts = new(DecompOpts)
	ret.Slicer = NewAnySlicer()
	ret.StopCond = StopOnNotPerfect

	ret.decomposer = NewDecomposer()
	ret.random = rand.New(rand.NewSource(0))

	ret.Target = NewMatrix()

	return ret
}

func (self *Scheduler) trim(data Matrix) {
	if !self.DoTrim {
		return
	}
	if Nhost <= 1 {
		return
	}

	ntrim := 0
	for i := 0; i < Nlane; i++ {
		if data[i] < LinkBw*MinDayLen && data[i] > 0 {
			data[i] = 0
			ntrim++
		}
	}
	logln("ntrim = ", ntrim)
	// println(ntrim)
}

func (self *Scheduler) _oldTrim(data Matrix) {
	if !self.DoTrim {
		return
	}
	if Nhost <= 1 {
		return
	}

	rowBuf := self.rowBuf
	colBuf := self.colBuf

	rowBuf.Clear()
	colBuf.Clear()
	total := PackBw * WeekLen
	lowWatermark := LinkBw * (MinDayLen - NightLen) * 6 / 10

	for i := 0; i < Nlane; i++ {
		row := i / Nhost
		col := i % Nhost
		d := data[i]
		if d >= lowWatermark {
			continue
		}

		if rowBuf[row]+d <= total && colBuf[col]+d <= total {
			data[i] = 0
			rowBuf[row] += d
			colBuf[col] += d
		}
	}
}

func scaleSlicesOld(slices []*Slice, target, grid uint64,
	minDayLen uint64) []*Slice {
	assert(grid > 0)
	assert(len(slices) > 0)

	target -= target % grid
	assert(uint64(len(slices))*NightLen < target)
	dayTarget := target - uint64(len(slices))*NightLen
	dayTarget -= dayTarget % grid

	total := uint64(0)
	for _, s := range slices {
		if s.Weight < grid {
			s.Weight = grid
		}
		s.Weight -= s.Weight % grid
		assert(s.Weight > NightLen)
		total += s.Weight - NightLen
		// println(s.Weight)
	}

	assert(minDayLen%grid == 0)

	total2 := uint64(0)
	for _, s := range slices {
		s.Weight = (s.Weight-NightLen)*dayTarget/total + NightLen
		s.Weight -= s.Weight % grid
		total2 += s.Weight
	}

	assert(target >= total2)

	residue := target - total2
	assert(residue%grid == 0)

	for residue > 0 {
		for _, s := range slices {
			s.Weight += grid
			residue -= grid
			if residue == 0 {
				break
			}
		}
	}

	return slices
}

func scaleSlicesOld2(slices []*Slice, target, grid uint64,
	minDayLen uint64) []*Slice {
	assert(grid > 0)
	assert(len(slices) > 0)

	target -= target % grid

	total := uint64(0)
	for _, s := range slices {
		if s.Weight < grid {
			s.Weight = grid
		}
		s.Weight -= s.Weight % grid
		total += s.Weight
	}

	assert(minDayLen%grid == 0)

	total2 := uint64(0)
	for _, s := range slices {
		s.Weight = s.Weight * target / total
		s.Weight -= s.Weight % grid
		total2 += s.Weight
	}

	assert(target >= total2)

	residue := target - total2
	assert(residue%grid == 0)

	for residue > 0 {
		for _, s := range slices {
			s.Weight += grid
			residue -= grid
			if residue == 0 {
				break
			}
		}
	}

	return slices
}

func removeMin(slices []*Slice, minDayLen uint64) ([]*Slice, bool) {
	minIndex := -1
	minWeight := uint64(0)
	for i, s := range slices {
		if s.Weight < minDayLen || minDayLen == 0 {
			if minIndex < 0 {
				minWeight = s.Weight
				minIndex = i
			} else if s.Weight < minWeight {
				minWeight = s.Weight
				minIndex = i
			}
		}
	}

	if minIndex < 0 {
		return slices, false // nothing to remove
	}

	last := len(slices) - 1
	if minIndex != last {
		slices[minIndex], slices[last] = slices[last], slices[minIndex]
	}
	return slices[:last], true // trim out
}

func scaleSlices(slices []*Slice, target, grid uint64,
	minDayLen uint64) []*Slice {
	assert(grid == 1)
	if len(slices) == 0 {
		return slices
	}

	var removed bool
	assert(minDayLen > NightLen)

	for {
		totalNight := uint64(len(slices)) * NightLen
		if totalNight >= target {
			// extreme case, too many days, just drop the tail
			slices = slices[:len(slices)-1]
			continue
		}

		scaleTarget := target - totalNight
		assert(scaleTarget > 0)

		total := uint64(0)
		for _, s := range slices {
			total += s.Weight
		}

		if total > scaleTarget {
			// linearly scale down
			canScaleDown := true
			for _, s := range slices {
				newWeight := s.Weight * scaleTarget / total
				if newWeight < minDayLen-NightLen {
					canScaleDown = false
					break
				}
			}

			if !canScaleDown {
				slices, _ = removeMin(slices, 0)
			} else {
				for _, s := range slices {
					s.Weight = s.Weight * scaleTarget / total
				}
			}
		} else if total < scaleTarget {
			// linearly scale up
			total2 := uint64(0)
			for _, s := range slices {
				s.Weight = s.Weight * scaleTarget / total
				total2 += s.Weight
			}

			assert(scaleTarget >= total2)

			residue := scaleTarget - total2

			for residue > 0 {
				for _, s := range slices {
					s.Weight += grid
					residue -= grid
					if residue == 0 {
						break
					}
				}
			}
		} else {
			// check if all met minDayLen
			slices, removed = removeMin(slices, minDayLen-NightLen)
			if !removed {
				for _, s := range slices {
					s.Weight += NightLen
				}
				return slices
			}
		}
	}
}

func findSlice(slices []*Slice, pm PortMap) (int, bool) {
	for i, s := range slices {
		if s.PortMap.Equal(pm) {
			return i, true
		}
	}
	return 0, false
}

func mergeSlices(slices []*Slice) []*Slice {
	ret := make([]*Slice, 0, len(slices))

	for _, s := range slices {
		i, found := findSlice(ret, s.PortMap)
		if found {
			// slice exists, then merge
			ret[i].Weight += s.Weight
		} else {
			// new slice, then append to results
			ret = append(ret, s)
		}
	}

	return ret
}

func (s *Scheduler) grid() uint64 {
	if !s.DoAlign {
		return TickPerGrid
	}

	if s.Grid%TickPerGrid != 0 {
		panic("bug")
	}

	if s.Grid > 0 {
		return s.Grid
	}

	if MinDayLen%TickPerGrid != 0 {
		panic("bug")
	}

	return MinDayLen
}

func (s *Scheduler) normalize(d Matrix, w uint64) {
	if w == WeekLen {
		return
	}
	d.Mul(WeekLen)
	d.Div(w)
}

func (s *Scheduler) interleave(slices []*Slice) []*Slice {
	if !s.DoInterleave {
		return slices
	}

	return s.interleaver.Interleave(slices)
}

func (s *Scheduler) align(d Matrix, grid uint64) {
	g := LinkBw * grid
	if !s.StarveAlign {
		d.NonZeroAtLeast(g)
	}
	d.RoundDiv(g)
	d.NormDown(WeekLen/grid, s.rowBuf) // TODO: use a bettern norm down
	d.Mul(grid)
}

func (s *Scheduler) merge(slices []*Slice) []*Slice {
	if !s.DoMerge {
		return slices
	}

	for _, slice := range slices {
		slice.PortMap.Mask(s.circs)
	}

	return mergeSlices(slices)
}

func (s *Scheduler) updateCircs(slices []*Slice) {
	s.circs.Clear()
	for _, slice := range slices {
		s.circs.MapInc(slice.PortMap)
	}
	s.circs.Binary()
}

func (s *Scheduler) discard(slices []*Slice) []*Slice {
	if !s.DoDiscard {
		return slices
	}

	ret := make([]*Slice, 0, len(slices))

	for _, slice := range slices {
		if slice.Weight < MinDayLen {
			continue
		}
		ret = append(ret, slice)
	}

	s.updateCircs(slices)

	for _, slice := range slices {
		slice.PortMap.Mask(s.circs)
	}

	// TODO: need to double think about this
	return mergeSlices(ret) // merge again
}

func (s *Scheduler) scale(slices []*Slice) []*Slice {
	if !s.DoScale {
		return slices
	}

	if TickPerGrid == 1 && s.DoAlexScale {
		return scaleSlices(slices, WeekLen, TickPerGrid, MinDayLen)
	}

	// return scaleSlicesOld(slices, WeekLen, TickPerGrid, MinDayLen)
	return scaleSlicesOld(slices, WeekLen, TickPerGrid, MinDayLen)
}

func (s *Scheduler) shuffle(slices []*Slice) []*Slice {
	if !s.DoShuffle {
		return slices
	}

	nday := len(slices)
	ret := make([]*Slice, nday)
	order := s.random.Perm(nday)
	for i, j := range order {
		ret[i] = slices[j]
	}

	return ret
}

func (s *Scheduler) idleWeek() ([]*Day, Matrix) {
	return []*Day{{NewMatrix(), WeekLen}}, NewMatrix()
}

func (s *Scheduler) countTicks(slices []*Slice) uint64 {
	s.ntick.Clear()
	weekLen := uint64(0)

	for _, slice := range slices {
		weekLen += slice.Weight
		if slice.Weight > NightLen {
			s.ntick.MapAdd(slice.PortMap, slice.Weight-NightLen)
		}
	}

	return weekLen
}

func (s *Scheduler) bandw(slices []*Slice) Matrix {
	ntick := s.countTicks(slices)
	s.updateCircs(slices)
	s.nbyte.Mmul(s.circs)

	// calculate bandwidth allocation
	if ntick != WeekLen {
		assert(!s.DoScale)
		s.nbyte.Mul(ntick)   // normalize to...
		s.nbyte.Div(WeekLen) // ...actual effective ticks
	}
	s.nbyte.MdivUp(s.ntick) // convert to bandw per tick

	// trim down when cannot satisfy
	if s.SafeBandw {
		s.nbyte.AtMost(LinkBw - PackBw)
	} else {
		s.nbyte.AtMost(LinkBw)
	}

	s.nbyte.NonZeroAtLeast(LinkBw - PackBw) // align up for unused part

	return s.nbyte
}

func (s *Scheduler) saveDayLens(slices []*Slice) {
	s.DayLens = make([]uint64, len(slices))

	for i, slice := range slices {
		s.DayLens[i] = slice.Weight
	}
}

// return nil where nothing to be scheduled
func (s *Scheduler) Schedule(r, d Matrix, w uint64) ([]*Day, Matrix) {
	// println("mustPerfect=", s.DecompOpts.MustPerfect)
	assert(d != nil)
	assert(WeekLen%TickPerGrid == 0)

	// Initialization
	grid := s.grid()
	assert(WeekLen%grid == 0)

	// teke the demand in
	copy(s.demand, d)
	s.normalize(s.demand, w) // normalize the demand to week length

	if r != nil && s.SchedResidue {
		s.demand.Madd(r)
	}

	// println("before trim:\n", s.demand.MatrixStr())
	s.trim(s.demand) // trim the small ones
	// println("after trim:\n", s.demand.MatrixStr())
	logln("after trimming:", s.demand.Clone().Div(LinkBw))
	copy(s.nbyte, s.demand)
	copy(s.circs, s.nbyte)
	s.circs.Binary()

	// aligning is mandatory, since it also converts the demand from bytes
	// to ticks. Also, it guarantees that the demand will be multiples of
	// TickPerGrid so that it can be used in the testbed well.
	//
	// When DoAlign == false, grid == TickPerGrid
	s.align(s.demand, grid) // convert the demand into grids

	/*
		The unit in s.demand is in ticks at this point
	*/

	// Stuff: fill the matrix into a doubly-stochastic one

	if s.DoStuff {
		norm := WeekLen
		if !s.FullStuff {
			norm = s.demand.MaxLineSum()
		}

		if s.DecompStuffer == nil {
			// fmt.Println("before stuffing:")
			// fmt.Println(s.demand.MatrixStr())

			if GridStuff(s.Stuffer, s.demand, norm, grid) == 0 {
				// nothing to schedule here, we can return early
				return s.idleWeek()
			}

			// fmt.Println("after stuffing:")
			// fmt.Println(s.demand.MatrixStr())
		} else {
			// re-stuff, we will stuff the matrix for
			// each level of decomposing inside the decomposer
			// so we don't need to do anything here

			if s.demand.Empty() {
				return s.idleWeek()
			}
		}
	}

	if !s.NoSaveTarget {
		copy(s.Target, s.demand)
	}
	s.demand.Div(grid)

	/*
		The unit in s.demand is in grids at the point
	*/

	// Decompose: slice the demand up
	assert(MinDayLen%grid == 0)
	min := MinDayLen / grid
	logln("grid:", grid)
	logln("min:", min)
	avg := AvgDayLen / grid
	if AvgDayLen%grid != 0 {
		avg++
	}

	s.DecompMin = min
	s.DecompWeek = WeekLen / grid
	if s.DoGreedy {
		s.DecompMax = 0
	} else {
		s.DecompMax = avg
	}

	/*
		if clock.T == 30000 {
			fmt.Println(clock.T)
			fmt.Println(s.demand.MatrixStr())
		}
	*/

	s.Slicer.Reset()
	ret := s.decomposer.Decompose(s.demand, s.DecompOpts)
	if grid != 1 {
		for _, slice := range ret {
			slice.Weight *= grid
			// slice.Weight += NightLen
		}
	}

	/*
		fmt.Printf("# %d:", len(ret))
		for _, slice := range ret {
			fmt.Printf(" %d", slice.Weight)
		}
		fmt.Println()
	*/

	if len(ret) == 0 {
		return s.idleWeek()
	}
	s.saveDayLens(ret)

	ret = s.merge(ret)
	ret = s.discard(ret)
	if len(ret) == 0 {
		return s.idleWeek()
	}
	ret = s.scale(ret)
	ret = s.interleave(ret)
	ret = s.shuffle(ret)

	var bandw Matrix
	if !s.SkipBandw {
		bandw = s.bandw(ret)
	}

	/*
		log.Println(d.MatrixStr())
		log.Print(s.Target.MatrixStr())
		log.Println(s.DayLens)
		log.Println()
	*/

	return slicesToDays(ret), bandw
}
