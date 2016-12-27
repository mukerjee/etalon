package main

import (
	"flag"
	"log"

	"react/conf"
)

var (
	outputPath = flag.String("dem", "demand", "demand output")
)

func main() {
	log.SetFlags(log.Llongfile)
	conf.Load()
	flag.Parse()

	d := makeDemand(conf.Conf.Demand, conf.Conf.Hosts)
	d.Save(*outputPath)
}
