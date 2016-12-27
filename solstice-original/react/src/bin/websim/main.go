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
	"react/sim/demand"
	"react/sim/diag"
	"react/sim/tors"
	"react/sim/web"
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
	pathDem  = flag.String("dem", "demand", "demand input")
	httpAddr = flag.String("http", ":8001", "http serve address")
	pathWeb  = flag.String("web", "www/websim", "static stuff address")
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

	t := hosts.Len() + WeekLen*3 // add 100 ticks for padding
	server, e := web.NewServer(*httpAddr, *pathWeb)
	noError(e)

	for server.WaitStart() {
		hosts.Restart()
		sw := tors.NewReactSwitch()

		var sched sim.Scheduler
		if !conf.Conf.Sched.Diag {
			sched = bvn.NewScheduler()
		} else {
			s := diag.NewScheduler()
			s.CleanOnIdle = conf.Conf.Sched.CleanOnIdle
			s.PureCircuit = conf.Conf.Sched.PureCircuit
			sched = s
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
		testbed.Display = server
		testbed.Stopper = server

		e = testbed.Run(t)
		noError(e)
	}
}
