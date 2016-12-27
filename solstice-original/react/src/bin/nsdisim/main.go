package main

import (
	"flag"
	"fmt"
	"os"
	// "runtime"
	// "runtime/pprof"

	"react/conf"
	"react/sim"
	"react/sim/bvn"
	. "react/sim/config"
	"react/sim/demand"
	"react/sim/diag"
	"react/sim/tors"
	"react/testb/setup"
)

func noError(e error) {
	if e != nil {
		fmt.Fprintln(os.Stderr, "error:", e)
		os.Exit(1)
	}
}

var (
	nhost     = flag.Int("nhost", 64, "number of hosts")
	pathDem   = flag.String("dem", "demand", "demand input")
	pathRep   = flag.String("rep", "", "replay output")
	pathSch   = flag.String("sch", "", "schedule output")
	schedPick = flag.String("sched", "sol", "scheduler (sol, dcirc, dhybr)")
	summary   = flag.Bool("summary", false, "print some summary info")
	pureCirc  = flag.Bool("purecirc", false, "schedule as a pure circuit")
	memProf   = flag.String("memProf", "", "memory profile")
	maxTime   = flag.Int("t", 0, "max time ticks, 0 for demand length")
	quiet     = flag.Bool("quiet", false, "do not print progress")
	noWarmUp  = flag.Bool("nowarmup", false, "skip warmup")
	noReactor = flag.Bool("noreactor", false, "do not use reactor settings when nhost=8")
	mordia    = flag.Bool("mordia", false, "use mordia time settings")
	eighty    = flag.Bool("eighty", false, "80/20 split on EPS and OCS")
)

func customConfig() {
	SetNhost(*nhost)
	TickPerGrid = 1
	if *nhost == 8 && !*noReactor {
		LinkBw = 1250
		PackBw = 125
	} else {
		LinkBw = 12500 // 100Gb

		if *eighty {
			PackBw = 2500 // 20Gb
		} else {
			PackBw = 1250 // 10Gb
		}
	}

	if *pureCirc {
		PackBw = 0
	}

	if *nhost == 8 && !*noReactor {
		// use shorter week length when host is 8
		// Shall we use mindspeed setup or mordia setup?
		if *mordia {
			NightLen = 30
			MinDayLen = 150
		} else {
			NightLen = 20
			MinDayLen = 40
		}
		AvgDayLen = 500
		WeekLen = 1500
		NicBufCover = 5000
	} else {
		NightLen = 20
		MinDayLen = 40
		AvgDayLen = 500
		WeekLen = 3000
		NicBufCover = 5000
	}

	Tracking = false
}

func main() {
	// runtime.GOMAXPROCS(4)
	flag.Parse()
	conf.Load()

	// config settings
	setup.ConfigSim()
	customConfig()

	// load in the demand
	hosts, e := demand.DemandHosts(*pathDem)
	noError(e)

	// t := hosts.Len() + WeekLen*3 // append 2 extra weeks
	t := hosts.Len()
	if *maxTime != 0 && uint64(*maxTime) < t {
		t = uint64(*maxTime)
	}
	noError(e)

	hosts.Restart()
	sw := tors.NewReactSwitch()

	var sched sim.Scheduler

	switch *schedPick {
	case "sol":
		s := bvn.NewScheduler()
		s.DoInterleave = false
		s.SafeBandw = false
		sched = s
	case "dcirc":
		s := diag.NewScheduler()
		s.SafeBandw = false
		s.PureCircuit = true
		sched = s
	case "dhybr":
		s := diag.NewScheduler()
		s.SafeBandw = false
		s.PureCircuit = false
		sched = s
	case "hotsp":
		s := bvn.NewHotSpotScheduler()
		sched = s
	default:
		panic(fmt.Errorf("unknown scheduler %s", *schedPick))
	}

	testbed := sim.NewTestbed(hosts, sw)
	testbed.Estimator = hosts
	testbed.Scheduler = sched
	if !*quiet {
		testbed.Progress = sim.NewLineProgress()
	}

	if *pathRep != "" {
		frep, e := os.Create(*pathRep)
		noError(e)
		defer func() { noError(frep.Close()) }()
		testbed.RecordTo(frep)
	}

	if *pathSch != "" {
		fsched, e := os.Create(*pathSch)
		noError(e)
		defer func() { noError(fsched.Close()) }()
		testbed.LogSchedTo(fsched)
	}

	if !*noWarmUp {
		testbed.WarmUp = WeekLen
	}

	e = testbed.Run(t)
	noError(e)

	if *summary {
		goodput := testbed.Goodput
		capacity := testbed.Capacity
		eff := float64(goodput) / float64(capacity) * 100
		fmt.Printf("goodput: %d/%d %.2f%%\n", goodput, capacity, eff)
	}

	if *memProf != "" {
		f, err := os.Create(*memProf)
		noError(err)
		// pprof.WriteHeapProfile(f)
		f.Close()
	}
}
