package main

import (
	"flag"
	"log"
	"time"

	"react/conf"
	"react/demand"
	"react/testb/ctrlh"
	"react/testb/headq"
	"react/testb/setup"
)

func noError(e error) {
	if e != nil {
		log.Fatalln(e)
	}
}

var (
	pathDem = flag.String("dem", "demand", "demand input")
	pathSch = flag.String("sch", "sim.sch", "schedule input")
	pathRep = flag.String("rep", "run.rep", "replay output")
)

func main() {
	conf.Load()
	flag.Parse()
	log.SetFlags(log.Ltime)

	hosts := conf.Conf.Hosts
	hq := headq.NewHeadqHosts(hosts)
	hq.SetVerbose(true)

	dem, e := demand.Load(*pathDem)
	noError(e)

	sch, e := ctrlh.LoadSchedule(*pathSch)
	noError(e)

	hq.Demand(dem)

	running := hq.Start() // Ready
	time.Sleep(3000 * time.Millisecond)

	// Go!
	builder := new(ctrlh.SchedBuilder)
	builder.Ports = ctrlh.MordiaPorts()
	// builder.NoSyncEndHosts = true
	builder.AllPacket = conf.Conf.Route.AllPacket
	e = builder.CompileTCP(setup.CtrlHostIP, sch)
	noError(e)

	hq.Join(running)

	e = hq.SaveReplay(*pathRep)
	noError(e)
}
