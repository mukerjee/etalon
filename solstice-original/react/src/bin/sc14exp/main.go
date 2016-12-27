package main

import (
	"fmt"
	"log"

	"react/sim/bvn"

	. "react/sim/config"
	. "react/sim/structs"
)

func main() {
	SetNhost(5)
	LinkBw = 1000
	PackBw = 100
	WeekLen = 100
	NightLen = 1
	MinDayLen = 2
	AvgDayLen = 4
	TickPerGrid = 1
	bvn.LogEnable = true
	log.SetFlags(0)

	m := Matrix([]uint64{
		0, 13, 10, 70, 4,
		1, 0, 14, 12, 69,
		65, 0, 0, 12, 14,
		15, 33, 2, 0, 11,
		12, 7, 3, 1, 0,
	})

	fmt.Println(m.MatrixStr())

	m.Mul(LinkBw)
	sched := bvn.NewScheduler()
	sched.DoInterleave = false
	days, bandw := sched.Schedule(nil, m, WeekLen)

	println(sched.Target.String())

	for _, day := range days {
		println(day.String())
	}

	println(bandw.MatrixStr())
}
