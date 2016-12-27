package main

import (
	"fmt"
	"math/rand"
	"os"
	"time"

	"react/sim/bvn"
	. "react/sim/config"
	. "react/sim/structs"
)

func newSched() (*bvn.Scheduler, *bvn.AnySlicer) {
	slicer := bvn.NewAnySlicer()
	sched := bvn.NewScheduler()
	sched.SafeBandw = true
	sched.SkipBandw = true
	sched.NoSaveTarget = true
	sched.DoTrim = true
	sched.DoAlign = false
	sched.DoMerge = false
	sched.DoDiscard = false
	sched.DoShuffle = false
	sched.DoInterleave = false
	sched.DoScale = true
	sched.DoGreedy = true
	sched.DoStuff = true
	sched.StopCond = bvn.StopOnNotPerfect
	sched.FullStuff = false
	sched.Slicer = slicer
	sched.Stuffer = bvn.NewSharpStuffer()

	return sched, slicer
}

func setup(nhost int) {
	SetNhost(nhost)
	LinkBw = 1000
	PackBw = 100
	WeekLen = 3000
	NightLen = 20
	MinDayLen = 40
	AvgDayLen = 40
}

const Nbig = 10

func makeDemand() Matrix {
	ret := NewMatrix()
	nbig := Nbig
	bw := (LinkBw - PackBw) / uint64(nbig)
	for i := 0; i < nbig; i++ {
		order := rand.Perm(Nhost)
		for row, col := range order {
			if row == col {
				continue
			}
			ret[row*Nhost+col] += bw * WeekLen
		}
	}

	/*
		nsmall := 3
		bw = PackBw / 2 / uint64(nsmall)
		for i := 0; i < nsmall; i++ {
			order := rand.Perm(Nhost)
			for row, col := range order {
				if row == col {
					continue
				}
				ret[row*Nhost+col] += bw * AvgDayLen
			}
		}
	*/

	return ret
}

const tries = 1000

func test(nhost int) []int {
	ret := make([]int, tries)
	daylens := make([]int, tries)
	times := make([]int, tries)

	setup(nhost)

	// sched, slicer := newSched()
	stuffer := bvn.NewSharpStuffer()
	slicer := bvn.NewAnySlicer()
	decomposer := bvn.NewDecomposer()
	decompOpts := new(bvn.DecompOpts)
	decompOpts.Slicer = slicer
	decompOpts.DecompMin = MinDayLen
	decompOpts.DecompWeek = WeekLen
	decompOpts.StopCond = bvn.StopOnNotPerfect

	for i := 0; i < tries; i++ {
		slicer.EdgesLooked = 0

		m := makeDemand()
		m.Div(LinkBw)
		// fmt.Println(m.MatrixStr())
		sp := m.Sparse()

		startTime := time.Now()
		stuffer.StuffSparse(sp, 0)
		slices := decomposer.DecomposeSparse(sp, decompOpts)
		runTime := time.Since(startTime)

		times[i] = int(runTime.Nanoseconds())
		daylens[i] = len(slices)
		if len(slices) > Nbig {
			panic(len(slices))
		}
		ret[i] = slicer.EdgesLooked
	}

	return ret
}

func main() {
	results := make([][]int, 0, 100)
	nhosts := []int{8, 16, 32, 64, 128, 256, 512, 1024}
	// nhosts := []int{8, 16, 32, 64, 128, 256}

	for _, nhost := range nhosts {
		fmt.Fprintln(os.Stderr, nhost)
		results = append(results, test(nhost))
	}

	for p, nhost := range nhosts {
		if p > 0 {
			fmt.Print(" ")
		}
		fmt.Print(nhost)
	}
	fmt.Println()

	for i := 0; i < tries; i++ {
		for p := range nhosts {
			if p > 0 {
				fmt.Print(" ")
			}
			fmt.Print(results[p][i])
		}
		fmt.Println()
	}
}
