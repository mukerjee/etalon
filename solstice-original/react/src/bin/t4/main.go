package main

import (
	"fmt"
	"math/rand"
	"os"
	"runtime"

	"react/sim"
	. "react/sim/blocks"
	"react/sim/bvn"
	"react/sim/clock"
	. "react/sim/config"
	"react/sim/eval"
	"react/sim/packet"
	. "react/sim/structs"
	"react/sim/tors"
)

func makeDemand() Matrix {
	demand := NewMatrix()
	for i := 0; i < Nhost; i++ {
		j := (i + 1) % Nhost
		demand[i*Nhost+j] = LinkBw * 1250

		j = (i + 2) % Nhost
		demand[i*Nhost+j] = LinkBw * 65

		j = (i + 3) % Nhost
		demand[i*Nhost+j] = LinkBw * 45

		j = (i + 4) % Nhost
		demand[i*Nhost+j] = LinkBw * 45
	}

	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			if i == j {
				continue
			}
			demand[i*Nhost+j] += uint64(rand.Intn(20 * int(LinkBw)))
		}
	}

	return demand
}

func ev(demand Matrix, sched sim.Scheduler) {

	rep := eval.EvalScheduler(sched, demand)
	fmt.Println(rep)
	/*
		fmt.Println("pack used:")
		fmt.Println(rep.PackUsed.MatrixStr())
		fmt.Println("unserved")
		fmt.Println(rep.Unserved.MatrixStr())
	*/
}

type staticDemand struct {
	d      Matrix
	maxLat uint64
	ndrop  int
	logTo  *os.File
}

func (sd *staticDemand) LogTo(path string) {
	var e error
	sd.logTo, e = os.Create(path)
	ne(e)
}

func (sd *staticDemand) Tick(sender Sender) {
	t := clock.T
	mod := t % WeekLen

	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			lane := i*Nhost + j
			size := sd.d[lane]
			if i == j && size > 0 {
				panic("bug")
			}

			pw := size / WeekLen
			m := size % WeekLen

			size = pw
			if mod < m {
				size++
			}

			if size > 0 {
				sender.Send(packet.NewPacket(size, lane))
			}
		}
	}
}

func (sd *staticDemand) Estimate() (Matrix, uint64) {
	ret := sd.d.Clone()
	return ret, WeekLen
}

func (sd *staticDemand) Push(p *packet.Packet) {
	if sd.d[p.Lane] < 30000 {
		if len(p.Footprint) <= 1 {
			panic("bug")
		}

		start := p.Footprint[0]
		end := p.Footprint[len(p.Footprint)-1]
		lat := end.TimeReached - start.TimeReached
		if lat > sd.maxLat {
			sd.maxLat = lat
		}

		if p.DroppedBy == packet.Nowhere {
			// do nothing
		} else if p.DroppedBy != packet.Downlink {
			fmt.Printf("dropped by %d\n", p.DroppedBy)
		} else {
			sd.ndrop++
		}

		if sd.logTo != nil {
			fmt.Fprintf(sd.logTo, "%d %d\n", clock.T, lat)
		}
		/*
			path := "packet"
			if !p.ViaPacket {
				path = "circuit"
			}
		*/
		// fmt.Printf("%s: %d\n", path, lat)
	}
}

func makeSolstice() sim.Scheduler {
	sched := bvn.NewScheduler()
	sched.DoInterleave = false
	sched.DoMerge = false
	sched.DoShuffle = false
	sched.SafeBandw = false
	return sched
}

func makeAlign() sim.Scheduler {
	sched := bvn.NewScheduler()
	sched.Slicer = bvn.NewAnySlicer()
	sched.SingleLevel = true
	sched.StopCond = bvn.StopOnNotPerfect
	sched.DoAlign = true
	sched.DoTrim = false
	sched.Grid = 100
	sched.StarveAlign = true
	sched.DoMerge = false
	sched.DoInterleave = false
	sched.DoShuffle = false
	sched.SafeBandw = false
	return sched
}

func ne(e error) {
	if e != nil {
		panic(e)
	}
}

func main() {
	runtime.GOMAXPROCS(4)

	SetNhost(8)
	WeekLen = 1500
	NightLen = 20

	MinDayLen = 40
	AvgDayLen = 40
	demand := makeDemand()
	fmt.Println("demand:")
	fmt.Println(demand.MatrixStr())
	fmt.Println(demand[48])

	fmt.Println("## Solstice")
	sched := makeSolstice()
	ev(demand, sched)

	fmt.Println("## Align")
	MinDayLen = 100
	AvgDayLen = 200
	sched = makeAlign()
	ev(demand, sched)

	// now let's run some simulation

	MinDayLen = 40
	AvgDayLen = 40
	hosts := &staticDemand{d: demand}
	hosts.LogTo("sols.lats")
	sw := tors.NewReactSwitch()
	sched = makeSolstice()

	testbed := sim.NewTestbed(hosts, sw)
	testbed.Estimator = hosts
	testbed.Scheduler = sched

	e := testbed.Run(WeekLen * 5)
	ne(e)

	goodput := testbed.Goodput
	capacity := uint64(Nhost) * LinkBw * testbed.EndTime
	eff := float64(goodput) / float64(capacity) * 100
	fmt.Printf("goodput: %d/%d %.2f%%\n", goodput, capacity, eff)
	fmt.Printf("max lat: %d   drop: %d\n", hosts.maxLat, hosts.ndrop)

	ne(hosts.logTo.Close())

	// now simulate with align
	MinDayLen = 100
	AvgDayLen = 200
	hosts = &staticDemand{d: demand}
	hosts.LogTo("align.lats")
	sw = tors.NewReactSwitch()
	sched = makeAlign()

	testbed = sim.NewTestbed(hosts, sw)
	testbed.Estimator = hosts
	testbed.Scheduler = sched

	e = testbed.Run(WeekLen * 5)
	ne(e)

	goodput = testbed.Goodput
	capacity = uint64(Nhost) * LinkBw * testbed.EndTime
	eff = float64(goodput) / float64(capacity) * 100
	fmt.Printf("goodput: %d/%d %.2f%%\n", goodput, capacity, eff)
	fmt.Printf("max lat: %d   drop: %d\n", hosts.maxLat, hosts.ndrop)
	ne(hosts.logTo.Close())
}
