package main

import (
	"flag"
	"log"
	"react/replay"
	"react/replay/web"
)

func noError(e error) {
	if e != nil {
		log.Fatalln(e)
	}
}

var (
	pathRep1  = flag.String("rep1", "sim.rep", "replay input")
	pathRep2  = flag.String("rep2", "test.rep", "replay input for diff")
	serveAddr = flag.String("http", ":8002", "serve address")
	channel   = flag.String("chan", "recv", "channel to display")
	webRoot   = flag.String("web", "www/webdiff", "web root")
	nbar      = flag.Int("nbar", 3000, "number of bars")
	win       = flag.Uint64("win", 20, "number of ticks per bar")
	start     = flag.Uint64("start", 0, "tick to start")
)

func main() {
	flag.Parse()

	rep1, e := replay.Load(*pathRep1)
	noError(e)

	rep2, e := replay.Load(*pathRep2)
	noError(e)

	log.Print("replay loaded")

	server := web.NewServer(rep1, rep2)

	slice := &web.Slice{
		Chan:       *channel,
		Start:      *start,
		Nbar:       *nbar,
		TickPerBar: *win,
	}
	server.Serve(*serveAddr, *webRoot, slice)
}
