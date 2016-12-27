package main

import (
	"flag"
	"log"

	"fmt"
	"react/replay"
)

func noError(e error) {
	if e != nil {
		log.Fatalln(e)
	}
}

func assert(cond bool) {
	if !cond {
		panic("bug")
	}
}

var (
	pathRep = flag.String("rep", "sim.rep", "replay input")
	tstart  = flag.Uint64("tstart", 4500, "time start")
	tend    = flag.Uint64("tend", 6000, "time end")
)

func main() {
	flag.Parse()

	rep, e := replay.Load(*pathRep)
	noError(e)

	c := rep.QueryChan("recv")
	assert(c >= 0)

	sum := uint64(0)
	capa := uint64(0)
	for t := *tstart; t < *tend; t++ {
		m := rep.Matrix(c, t)
		sum += m.Sum()
		capa += rep.LinkBw * uint64(rep.Nhost())
	}

	fmt.Println(sum)
}
