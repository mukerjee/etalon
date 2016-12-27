package main

import (
	"fmt"
	"log"
	"math/rand"

	"react/sim/bvn"

	. "react/sim/config"
	. "react/sim/structs"
)

func main() {
	SetNhost(8)
	LinkBw = 1000
	PackBw = 100
	WeekLen = 1000
	NightLen = 10
	MinDayLen = 20
	AvgDayLen = 20
	TickPerGrid = 1
	bvn.LogEnable = true
	log.SetFlags(0)

	schedName := "islip"
	switch schedName {
	case "bvn":
		MinDayLen = 11
	case "islip":
		AvgDayLen = 20
	}

	m := Matrix([]uint64{
		30, 30, 30, 270, 530, 0, 30, 30,
		30, 30, 270, 530, 0, 30, 30, 30,
		270, 530, 0, 30, 30, 30, 30, 30,
		30, 270, 530, 0, 30, 30, 30, 30,
		530, 0, 30, 30, 30, 30, 30, 270,
		0, 30, 30, 30, 30, 30, 270, 530,
		30, 30, 30, 30, 270, 530, 0, 30,
		30, 30, 30, 30, 30, 270, 530, 0,
	})

	for i, d := range m {
		if d != 0 {
			m[i] = d + uint64(rand.Intn(10)) - 5
		}
	}

	fmt.Println(m.MatrixStr())

	m.Mul(LinkBw)
	sched := bvn.NewScheduler()
	sched.DoInterleave = false

	switch schedName {
	case "sols":
		// do nothing
	case "islip":
		slicer := bvn.NewIslipSlicer()
		slicer.Iteration = 4
		sched.Slicer = slicer
		sched.DoMerge = false
		sched.DoStuff = false
		sched.DoGreedy = true
		sched.SingleLevel = true
		sched.StopCond = bvn.StopOnTimeUsedUp
	case "bvn":
		slicer := bvn.NewAnySlicer()
		sched.Stuffer = bvn.NewQuickStuffer()
		sched.Slicer = slicer
		sched.SingleLevel = true
		sched.StopCond = bvn.StopOnNotPerfect
	}

	days, bandw := sched.Schedule(nil, m, WeekLen)

	println(sched.Target.String())

	println(len(days), "days")
	for _, day := range days {
		println(day.String())
	}

	println(bandw.MatrixStr())
}
