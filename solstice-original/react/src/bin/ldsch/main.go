package main

import (
	"flag"
	"log"

	"react/conf"
	"react/testb/ctrlh"
	"react/testb/setup"
)

func noError(e error) {
	if e != nil {
		log.Fatalln(e)
	}
}

var (
	pathSch   = flag.String("sch", "sim.sch", "schedule input")
	allPacket = flag.Bool("allpack", false, "all on packet switch")
)

func main() {
	conf.Load()
	flag.Parse()
	log.SetFlags(log.Ltime)

	// Load
	sch, e := ctrlh.LoadSchedule(*pathSch)
	noError(e)

	// Go!
	builder := new(ctrlh.SchedBuilder)
	builder.Ports = ctrlh.MordiaPorts()
	// builder.NoSyncEndHosts = true
	builder.AllPacket = *allPacket
	e = builder.CompileTCP(setup.CtrlHostIP, sch)
	noError(e)
}
