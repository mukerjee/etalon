package main

import (
	"flag"
	"fmt"
	"os"
	"runtime"

	"react/conf"
	"react/sim"
	"react/sim/bvn"
	. "react/sim/config"
	"react/sim/csol"
	"react/sim/demand"
	"react/sim/diag"
	"react/sim/tors"
	"react/sim/win"
	"react/testb/setup"
)

func noError(e error) {
	if e != nil {
		fmt.Fprintln(os.Stderr, "error:", e)
		os.Exit(1)
	}
}

var (
	pathDem = flag.String("dem", "demand", "demand input")
	pathRep = flag.String("rep", "replay", "replay output")
	pathSch = flag.String("sch", "sched", "schedule output")
	summary = flag.Bool("summary", false, "print some summary info")
)

func main() {
	runtime.GOMAXPROCS(4)
	flag.Parse()
	conf.Load()

	// config settings
	setup.ConfigSim()

	// load in the demand
	hosts, e := demand.DemandHosts(*pathDem)
	noError(e)

	t := hosts.Len() + WeekLen*3 // append 2 extra weeks
	noError(e)

	hosts.Restart()
	sw := tors.NewReactSwitch()

	var sched sim.Scheduler

	if conf.Conf.Sched.UseC {
		sched = csol.New()
	} else if conf.Conf.Sched.Diag {
		s := diag.NewScheduler()
		s.CleanOnIdle = conf.Conf.Sched.CleanOnIdle
		s.PureCircuit = conf.Conf.Sched.PureCircuit
		sched = s
	} else {
		sched = bvn.NewScheduler()
	}

	testbed := sim.NewTestbed(hosts, sw)
	switch conf.Conf.Estimator {
	case "future":
		testbed.Estimator = hosts
	case "halfwin":
		window := win.NewWindowEstimator(WeekLen / 2)
		testbed.Monitor = window
		testbed.Estimator = window
	default:
		panic(fmt.Errorf("unknown estimator: %s", conf.Conf.Estimator))
	}

	testbed.Scheduler = sched

	frep, e := os.Create(*pathRep)
	noError(e)
	defer func() { noError(frep.Close()) }()
	testbed.RecordTo(frep)

	fsched, e := os.Create(*pathSch)
	noError(e)
	defer func() { noError(fsched.Close()) }()
	testbed.LogSchedTo(fsched)

	e = testbed.Run(t)
	noError(e)

	if *summary {
		goodput := testbed.Goodput
		capacity := uint64(Nhost) * LinkBw * testbed.EndTime
		eff := float64(goodput) / float64(capacity) * 100
		fmt.Printf("goodput: %d/%d %.2f%%\n", goodput, capacity, eff)
	}
}
