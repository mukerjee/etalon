package main

import (
	"fmt"
	"log"
	"math/rand"
	"os"
	"time"

	"react/sim/bvn"
	. "react/sim/config"
	"react/sim/drainer"
	. "react/sim/structs"
)

/*
func newSched() (*bvn.Scheduler, *bvn.AnySlicer) {
	slicer := bvn.NewAnySlicer()
	sched := bvn.NewScheduler()
	sched.SafeBandw = false
	sched.DoTrim = true
	sched.DoAlign = false
	sched.DoMerge = true
	sched.DoDiscard = false
	sched.DoShuffle = false
	sched.DoInterleave = false
	sched.DoGreedy = true
	sched.DoStuff = true
	sched.StopCond = bvn.StopOnNotPerfect
	sched.FullStuff = false
	sched.Slicer = slicer
	sched.Stuffer = bvn.NewSharpStuffer()

	return sched, slicer
}
*/

func makeDemand(setup *Setup) Matrix {
	if setup.Demand == "unify" {
		return makeUnifyDemand(setup)
	} else if setup.Demand == "rand" {
		if setup.Nsmall == 0 {
			return makeRandDemand(setup)
		} else {
			return makeRandSkewDemand(setup)
		}
	} else {
		panic(fmt.Errorf("unknown demand: %s", setup.Demand))
	}
}

func randLane() int {
	if Nhost <= 1 {
		panic("bug")
	}

	row := rand.Intn(Nhost)
	col := (row + 1 + rand.Intn(Nhost-1)) % Nhost
	return row*Nhost + col
}

func makeRandDemand(setup *Setup) Matrix {
	cross := drainer.NewCrossLimits(setup.FillBw * WeekLen)
	d := drainer.NewDrainer(cross.Limits)

	count := NewMatrix()
	budget := NewMatrix()
	nflow := setup.Nbig

	for i := 0; i < nflow; i++ {
		lane := randLane()
		count[lane]++
		budget[lane] = setup.FillBw * WeekLen
	}

	ret := d.WeightDrain(budget, count).Clone()

	noise := setup.Noise * int(WeekLen)
	for i, r := range ret {
		if r > 0 {
			ret[i] += uint64(rand.Intn(noise))
			halfNoise := uint64(noise / 2)
			if r > halfNoise {
				ret[i] -= halfNoise
			} else {
				ret[i] = 0
			}
		}
	}
	// ret.Mul(WeekLen)

	return ret
}

func addNoise(a uint64, noise int) uint64 {
	if noise == 0 {
		return a
	}
	a += uint64(rand.Intn(noise))
	halfNoise := uint64(noise / 2)
	if a > halfNoise {
		return a - halfNoise
	}

	return 0
}

func makeRandSkewDemand(setup *Setup) Matrix {
	cross := drainer.NewCrossLimits(setup.FillBw * WeekLen)
	d := drainer.NewDrainer(cross.Limits)

	budget := NewMatrix()
	nsmall := setup.Nsmall
	smallCount := NewMatrix()

	smallBw := setup.FillBw * uint64(setup.FracSmall) /
		uint64(setup.FracNorm)

	if smallBw > 0 {
		for i := 0; i < nsmall; i++ {
			lane := randLane()
			budget[lane] += addNoise(smallBw*WeekLen,
				setup.Noise*int(WeekLen))
			if budget[lane] > setup.FillBw*WeekLen {
				budget[lane] = setup.FillBw * WeekLen
			}
			smallCount[lane]++
		}
	}

	bigCount := NewMatrix()
	nflow := setup.Nbig
	for i := 0; i < nflow; i++ {
		lane := randLane()
		budget[lane] = setup.FillBw * WeekLen
		bigCount[lane]++
	}

	ret := d.Drain(budget).Clone()

	rowSum := NewVector()
	colSum := NewVector()
	bigCount.Sums(rowSum, colSum)
	// println("big:", rowSum.Max(), colSum.Max())
	smallCount.Sums(rowSum, colSum)
	// println("small:", rowSum.Max(), colSum.Max())

	/*
		noise := setup.Noise
		for i, r := range ret {
			if r > 0 {
				ret[i] += uint64(rand.Intn(noise))
				halfNoise := uint64(noise / 2)
				if r > halfNoise {
					ret[i] -= halfNoise
				} else {
					ret[i] = 0
				}
			}
		}
	*/

	return ret
}

var oracleDays []*Day
var oracleBandw Matrix

func nbigCirc(nbig int, bigBw uint64) int {
	max := uint64(0)
	maxX := nbig
	for x := 0; x <= nbig; x++ {
		nights := uint64(x) * NightLen
		circMax := uint64(0)
		if WeekLen > nights {
			circMax = LinkBw * (WeekLen - nights)
		}
		circ := bigBw * uint64(x)
		if circ > circMax {
			circ = circMax
		}

		packMax := PackBw * WeekLen
		pack := bigBw*uint64(nbig) - circ // all the rests
		// pack := bigBw * uint64(nbig-x)
		if pack > packMax {
			pack = packMax
		}

		served := circ + pack
		if served >= max {
			maxX = x
			max = served
		}
		// println(nbig, x, served, circ, pack)
	}

	return maxX
}

func makeUnifyDemand(setup *Setup) Matrix {
	nsmall := setup.Nsmall
	nbig := setup.Nbig
	fracNorm := uint64(setup.FracNorm)
	fracSmall := uint64(setup.FracSmall)
	fracBig := uint64(setup.FracNorm - setup.FracSmall)
	totalBw := setup.FillBw * WeekLen
	smallBw := uint64(0)
	bigBw := uint64(0)
	noise := setup.Noise * int(WeekLen)

	if nsmall > 0 {
		smallBw = totalBw * fracSmall / fracNorm / uint64(nsmall)
	}
	if nbig > 0 {
		bigBw = totalBw * fracBig / fracNorm / uint64(nbig)
	}
	oracleDays = make([]*Day, 0, nbig+nsmall)
	oracleBandw = NewMatrix()
	ret := NewMatrix()

	for i := 0; i < nsmall+nbig; i++ {
		order := rand.Perm(Nhost)
		bw := bigBw
		if i < nsmall {
			bw = smallBw
		}

		oracleSched := NewMatrix()
		for row, col := range order {
			lane := row*Nhost + col
			oracleSched[lane] = 1
			oracleBandw[lane] = LinkBw
		}

		if fracSmall > fracNorm/20 {
			if WeekLen <= NightLen*uint64(nbig+nsmall) {
				panic("demand not admissable")
			}
			effWeekLen := WeekLen - NightLen*uint64(nbig+nsmall)
			dayLen := bw*effWeekLen/setup.FillBw + NightLen
			/*
				if nsmall == 0 {
					dayLen = WeekLen / uint64(nbig)
				}
			*/
			oracleDay := NewDay(oracleSched, dayLen)
			oracleDays = append(oracleDays, oracleDay)
		} else if i >= nsmall {
			if WeekLen <= NightLen*uint64(nbig) {
				panic("demand not admissalbe")
			}

			dayLen := WeekLen / uint64(nbig)
			oracleDay := NewDay(oracleSched, dayLen)
			oracleDays = append(oracleDays, oracleDay)
		}

		for row, col := range order {
			if setup.ClearDiag && row == col {
				continue
			}

			thisBw := bw
			if bw == 0 {
				continue
			}

			if noise > 0 {
				thisBw += uint64(rand.Intn(noise))
				halfNoise := uint64(noise / 2)
				if thisBw > halfNoise {
					thisBw -= halfNoise
				} else {
					thisBw = 0
				}
			}
			ret[row*Nhost+col] += thisBw
		}
	}

	if nsmall == 0 {
		if setup.HybridOracle {
			ncirc := nbigCirc(nbig, bigBw)
			oracleDays = oracleDays[0:ncirc]
		}

		ndays := len(oracleDays)
		residue := WeekLen % uint64(ndays)
		for i, day := range oracleDays {
			day.Len = WeekLen / uint64(ndays)
			if uint64(i) < residue {
				day.Len++
			}
		}
	}

	return ret
}

type Result struct {
	id      int
	nedge   int
	nbyte   uint64
	nbyte2  uint64
	nbyte3  uint64
	ndemand uint64
	nday    int
	t       time.Duration
}

func nbyte(m Matrix, days []*Day, bandw Matrix) (uint64, uint64, uint64) {
	served := NewMatrix()
	noncirc := NewMatrix()
	unserved := NewMatrix()

	t := NewMatrix()

	for _, day := range days {
		if day.Len <= NightLen {
			continue
		}
		copy(t, day.Sched)
		t.Mul(day.Len - NightLen)
		t.Mmul(bandw)
		served.Madd(t)
	}

	served.MatMost(m)
	for i, s := range served {
		if s < m[i] {
			unserved[i] = m[i] - s
			if s == 0 {
				noncirc[i] = m[i]
			}
		}
	}

	cross := drainer.NewCrossLimits(PackBw * WeekLen)
	d := drainer.NewDrainer(cross.Limits)

	npack1 := d.Drain(noncirc).Sum()
	npack2 := d.Drain(unserved).Sum()
	ret := served.Sum()

	return ret, ret + npack1, ret + npack2
}

func clearEdgeLooked(slicer bvn.Slicer) {
	if slicer == nil {
		return
	}

	anySlicer, okay := slicer.(*bvn.AnySlicer)
	if !okay {
		return
	}
	anySlicer.EdgesLooked = 0
}

func edgeLooked(slicer bvn.Slicer) int {
	if slicer == nil {
		return 0
	}

	anySlicer, okay := slicer.(*bvn.AnySlicer)
	if !okay {
		return 0
	}
	return anySlicer.EdgesLooked
}

func printDayLengths(days []*Day) {
	fmt.Printf("%d:", len(days))
	for _, d := range days {
		fmt.Printf(" %d", d.Len)
	}
	fmt.Println()
}

func test(setup *Setup) []*Result {
	tries := setup.Tries
	ret := make([]*Result, tries)
	setup.Apply()

	sched, slicer := setup.MakeScheduler()
	for i := 0; i < tries; i++ {
		clearEdgeLooked(slicer)

		m := makeDemand(setup)
		orig := m.Clone()
		timeStart := time.Now()
		var days []*Day
		var bandw Matrix
		if setup.Scheduler == "oracle" {
			if setup.Demand != "unify" {
				panic("orcale only handles unify demand")
			}
			days, bandw = oracleDays, oracleBandw
		} else {
			days, bandw = sched.Schedule(nil, m, WeekLen)
		}
		// fmt.Println(orig.MatrixStr())
		// printDayLengths(days)
		// fmt.Println(bandw.MatrixStr())
		// fmt.Println()
		computeTime := time.Since(timeStart)

		r := new(Result)
		r.nedge = edgeLooked(slicer)
		r.nday = len(days)
		r.nbyte, r.nbyte2, r.nbyte3 = nbyte(orig, days, bandw)
		r.ndemand = orig.Sum()
		r.t = computeTime

		ret[i] = r
	}

	return ret
}

func main() {
	setup, e := TryLoad()
	if e != nil {
		log.Fatal(e)
	}

	fout, e := os.Create("dat")
	if e != nil {
		log.Fatal(e)
	}

	results := make([][]*Result, 0, 1000)
	ids := make([]int, 0, 1000)

	for setup.Next() {
		fmt.Fprintf(os.Stderr, "scan = %d\n", setup.ScanId)
		results = append(results, test(setup))
		ids = append(ids, setup.ScanId)
	}

	for i, x := range ids {
		if i > 0 {
			fmt.Fprint(fout, " ")
		}
		fmt.Fprintf(fout, "x%d.t x%d.tday x%d.nedge x%d.nday "+
			"x%d.nbyte x%d.ndem x%d.ndem2 x%d.ndem3",
			x, x, x, x, x, x, x, x)
	}
	fmt.Fprintln(fout)

	for i := 0; i < setup.Tries; i++ {
		for p, result := range results {
			if p > 0 {
				fmt.Fprint(fout, " ")
			}
			res := result[i]
			fmt.Fprintf(fout, "%d %d %f %d %f %f %f %f",
				res.t.Nanoseconds(),
				res.t.Nanoseconds()/int64(res.nday),
				float64(res.nedge)/float64(Nhost),
				res.nday,
				float64(res.nbyte)/float64(Nhost)/
					float64(WeekLen)/float64(LinkBw),
				float64(res.nbyte)/float64(res.ndemand),
				float64(res.nbyte2)/float64(res.ndemand),
				float64(res.nbyte3)/float64(res.ndemand),
			)
		}
		fmt.Fprintln(fout)
	}
}
