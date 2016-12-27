package main

import (
	"flag"
	"fmt"
	"os"

	. "bin/dctcp/rc"

	"react/sim"
	"react/sim/alexf"
	"react/sim/bvn"
	"react/sim/config"
	"react/sim/density"
	"react/sim/flows"
	"react/sim/flows/dctcp"
	"react/sim/flows/simple"
	"react/sim/simlog"
	"react/sim/tors"
	"react/sim/win"
)

func noError(e error) {
	if e != nil {
		panic(e)
	}
}

var (
	rc  = TheRC()
	frc = flag.String("rc", "sim.rc", "simulation configuration file")
)

func setupConfig() {
	config.SetNhost(rc.Nhost)
	config.WeekLen = 3000
	config.MinDayLen = 40
	if rc.Sched == "align" {
		config.MinDayLen = 100
	}
	config.AvgDayLen = 200
	config.NightLen = 20

	if rc.Sched == "alexf" {
		config.WeekLen = 600
	}

	config.TickPerGrid = 1
	config.LinkBw = rc.LinkBw * rc.UnitScaler * rc.LinkScaler
	config.PackBw = rc.PackBw * rc.UnitScaler * rc.LinkScaler

	dctcp.QuerySizeScaler = rc.UnitScaler
	dctcp.QueryFreqScaler = float64(rc.LinkScaler)

	dctcp.FlowSizeScaler = rc.UnitScaler
	dctcp.FlowFreqScaler = float64(rc.LinkScaler)
	dctcp.SmallFlowThres = 5e4 * rc.UnitScaler

	simple.BigFlowSizeScaler = rc.UnitScaler
	simple.BigFlowFreqScaler = float64(rc.LinkScaler)
	simple.SmallFlowSizeScaler = rc.UnitScaler
	simple.SmallFlowFreqScaler = float64(rc.LinkScaler) * rc.SmallFlowFreq

	density.BigThres = 150000 * rc.UnitScaler * rc.LinkScaler
}

func workload() flows.FlowWorkload {
	switch rc.Workload {
	case "dctcp":
		workload := dctcp.NewDCTCPWorkloadScaled(0, rc.LoadScaler)
		workload.NoQueries = rc.NoQueries
		workload.NoFlows = rc.NoFlows
		workload.NoBig = rc.NoBig
		return workload
	case "simple":
		workload := simple.New(0, rc.LoadScaler)
		workload.NoBig = rc.NoBig
		workload.NoSmall = rc.NoSmall
		return workload
	default:
		panic("unknown workload")
	}
}

func main() {
	flag.Parse()
	LoadRC(*frc)
	PrintRC()
	setupConfig()

	/*
		server, e := web.NewServer("localhost:8000", "www/websim")
		noError(e)
	*/

	// for server.WaitStart() {

	workload := workload()
	driver := flows.NewFlowDriver()
	driver.Workload = workload
	if rc.FlowLog != "" {
		fout, e := os.Create(rc.FlowLog)
		noError(e)
		defer fout.Close()
		driver.LogTo = fout
	}

	keeper := flows.NewFlowKeeper()
	keeper.AssignDriver(driver)

	var s sim.Switch
	var sched sim.Scheduler
	switch rc.Switch {
	case "pack":
		s = tors.NewPacketSwitch()
	case "circ":
		panic("TODO")
	case "react", "preact", "dreact":
		if rc.Switch == "react" {
			s = tors.NewReactSwitch()
		} else if rc.Switch == "preact" {
			s = tors.NewPreactSwitch()
		} else {
			assert(rc.Switch == "dreact")
			s = tors.NewDreactSwitch()
		}
		switch rc.Sched {
		case "sol": // solstice
			rs := bvn.NewScheduler()
			rs.DoInterleave = false
			rs.SafeBandw = rc.SafeBandw
			// rs.DoShuffle = false
			sched = rs
		case "align":
			rs := bvn.NewScheduler()
			rs.SingleLevel = true
			rs.DoAlign = true
			rs.DoTrim = true
			if config.WeekLen != 3000 {
				panic("align only works for weeklen 3000")
			}
			rs.Grid = 100 // we can try with other girds
			rs.StarveAlign = true
			sched = rs
		case "alexf":
			rs := alexf.NewScheduler()
			sched = rs
		default:
			panic("bug")
		}
	default:
		fmt.Printf("unknown switch %q\n", rc.Switch)
		panic("bug")
	}

	wind := win.NewWindowTracker(config.WeekLen)
	windLog := simlog.NewMatWriter(rc.WinLog)
	wind.Logger = windLog
	defer windLog.Close()

	testbed := sim.NewTestbed(keeper, s)
	if rc.Switch != "pack" {
		testbed.Estimator = wind
	}
	testbed.Monitor = wind
	progress := sim.NewLineProgress()
	progress.Flows = keeper
	testbed.Progress = progress

	if sched != nil {
		testbed.Scheduler = sched
	}
	// testbed.Display = server
	// testbed.Stopper = server

	/*
		frep, e := os.Create("rep")
		noError(e)
		defer func() { noError(frep.Close()) }()
		testbed.RecordTo(frep)
	*/

	if rc.SchedLog != "" {
		fsched, e := os.Create(rc.SchedLog)
		noError(e)
		defer func() { noError(fsched.Close()) }()
		testbed.LogSchedTo(fsched)
	}

	e := testbed.Run(rc.Tmax)
	noError(e)

	if rc.Summary != "" {
		fout, e := os.Create(rc.Summary)
		noError(e)
		noError(testbed.Summarize(fout))
		noError(fout.Close())
	}

	fmt.Print(wind.D.String())
}
