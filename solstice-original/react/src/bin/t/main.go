package main

import (
	"fmt"
	// "os"

	"react/sim"
	// "react/sim/bvn"
	"react/sim/config"
	"react/sim/flows"
	"react/sim/flows/dctcp"
	"react/sim/tors"
	// "react/sim/web"
	"react/sim/simlog"
	"react/sim/win"

	"react/sim/density"
)

func noError(e error) {
	if e != nil {
		panic(e)
	}
}

func setupConfig() {
	config.SetNhost(64)
	config.WeekLen = 3000
	config.MinDayLen = 40
	config.AvgDayLen = 200
	config.NightLen = 20
	config.TickPerGrid = 1
	config.LinkBw = 1250
	config.PackBw = 120

	density.BigThres = 150000
}

func main() {
	setupConfig()

	/*
		server, e := web.NewServer("localhost:8000", "www/websim")
		noError(e)
	*/

	// for server.WaitStart() {

	workload := dctcp.NewDCTCPWorkload(0)
	driver := flows.NewFlowDriver()
	driver.Workload = workload

	keeper := flows.NewFlowKeeper()
	keeper.AssignDriver(driver)

	/*
		sched := bvn.NewScheduler()
		sched.DoInterleave = false
		sched.SafeBandw = false
	*/

	// sw := tors.NewReactSwitch()
	sw := tors.NewPacketSwitch()
	wind := win.NewWindowTracker(config.WeekLen)
	windLog := simlog.NewMatWriter("win.log")
	wind.Logger = windLog
	defer windLog.Close()

	testbed := sim.NewTestbed(keeper, sw)
	// testbed.Estimator = wind
	testbed.Monitor = wind
	testbed.Progress = sim.NewLineProgress()
	// testbed.Scheduler = sched
	// testbed.Display = server
	// testbed.Stopper = server

	/*
		frep, e := os.Create("rep")
		noError(e)
		defer func() { noError(frep.Close()) }()
		testbed.RecordTo(frep)
	*/

	e := testbed.Run(30e6)
	noError(e)

	fmt.Print(wind.D.String())
}
