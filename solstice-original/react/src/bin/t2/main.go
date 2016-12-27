package main

import (
	"fmt"
	"log"
	"time"

	"react/sim/bvn"

	. "react/sim/config"
	. "react/sim/structs"

	"react/sim/drainer"

	"math/rand"
)

type Flows struct {
	Nflow float64
	Noise uint64
	Max   uint64

	rand *rand.Rand
}

func NewFlows() *Flows {
	ret := new(Flows)
	ret.Nflow = 1.0
	ret.Noise = PackBw / 2
	ret.Max = LinkBw - PackBw

	return ret
}

func randLane(r *rand.Rand) int {
	if Nhost <= 1 {
		panic("bug")
	}

	row := r.Intn(Nhost)
	col := (row + 1 + r.Intn(Nhost-1)) % Nhost
	return row*Nhost + col
}

func (self *Flows) randLane() int {
	return randLane(self.rand)
}

func (self *Flows) makeFlows() (budget, count Matrix) {
	count = NewMatrix()
	budget = NewMatrix()

	nflow := int(self.Nflow * float64(Nhost))

	for i := 0; i < nflow; i++ {
		lane := self.randLane()
		count[lane]++
		budget[lane] = self.Max
	}

	return
}

func (self *Flows) _make() Matrix {
	cross := drainer.NewCrossLimits(self.Max)
	d := drainer.NewDrainer(cross.Limits)
	budget, count := self.makeFlows()
	return d.WeightDrain(budget, count).Clone()
}

func (self *Flows) Make(r *rand.Rand) Matrix {
	self.rand = r
	ret := self._make()

	return ret
}

func main() {
	SetNhost(4)
	LinkBw = 10
	PackBw = 0
	WeekLen = 10
	NightLen = 0
	MinDayLen = 1
	AvgDayLen = 2
	TickPerGrid = 1
	bvn.LogEnable = true
	log.SetFlags(0)
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	flows := NewFlows()
	flows.Nflow = 3
	flows.Noise = 0

	m := flows.Make(r)
	/* m = []uint64{
		1, 10, 20, 30, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
	}; */

	fmt.Println(m.MatrixStr())

	m.Mul(LinkBw)
	sched := bvn.NewScheduler()
	sched.DoTrim = false
	sched.DoInterleave = false
	sched.DoStuff = false
	slicer := bvn.NewMaxSumSlicer()
	// slicer.AcceptImperfect = true
	sched.StopCond = bvn.StopOnZero
	sched.Slicer = slicer
	sched.SingleLevel = true
	sched.DoScale = false

	days, _ := sched.Schedule(nil, m, WeekLen)

	println(sched.Target.MatrixStr())

	target := sched.Target.Clone()

	total := uint64(0)
	for _, day := range days {
		total += day.Len
		println("day:")
		println(day.Len)
		println(day.Sched.MatrixStr())
		t := day.Sched.Clone()
		t.Mul(day.Len)
		target.Msub(t)
		println("left:")
		println(target.MatrixStr())
	}
	// println(bandw.MatrixStr())
	println(total)
	if total > WeekLen {
		panic("found it")
	}
}
