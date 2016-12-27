package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"log"
	"net"
	"time"

	"react/conf"
	"react/demand"
	"react/testb/headq"
	"react/testb/setup"
)

func noError(e error) {
	if e != nil {
		log.Fatalln(e)
	}
}

var (
	pathDem    = flag.String("dem", "demand", "demand input")
	pathRep    = flag.String("rep", "run.rep", "replay output")
	r0Ctrl     = flag.Bool("r0", false, "if using r0 as control host")
	noEndhosts = flag.Bool("ctrlonly", false, "talk to the control host only")

	enc = binary.LittleEndian
)

func matBuf(nhost int, m []demand.Vector) []byte {
	ret := make([]byte, nhost*nhost*8)
	pt := 0
	for _, v := range m {
		for _, i := range v {
			enc.PutUint64(ret[pt:pt+8], i)
			pt += 8
		}
	}
	if pt != len(ret) {
		panic("nhost mismatch")
	}
	return ret
}

func runDemand(d *demand.Demand) error {
	ctrlhost := setup.CtrlHostIP
	if *r0Ctrl {
		ctrlhost = "172.22.16.50"
	}

	a := fmt.Sprintf("%s:%d", ctrlhost, setup.CtrlPort)
	_conn, e := net.DialTimeout("tcp4", a, time.Second)
	if e != nil {
		return e
	}
	conn := _conn.(*net.TCPConn)

	c := d.WindChan(setup.WeekLen)
	nhost := d.Nhost()

	for m := range c {
		buf := matBuf(nhost, m)
		_, e := conn.Write(buf)
		if e != nil {
			conn.Close()
			return e
		}
	}

	return conn.Close()
}

func main() {
	conf.Load()
	flag.Parse()
	log.SetFlags(log.Ltime)

	hosts := conf.Conf.Hosts
	var hq *headq.Headq

	if !*noEndhosts {
		hq = headq.NewHeadqHosts(hosts)
		hq.SetVerbose(true)
	}

	dem, e := demand.Load(*pathDem)
	noError(e)

	if !*noEndhosts {
		hq.Demand(dem)
	}

	var running headq.Runnings
	if !*noEndhosts {
		running = hq.Start() // Ready
		time.Sleep(3000 * time.Millisecond)
	}

	e = runDemand(dem)
	noError(e)

	if !*noEndhosts {
		hq.Join(running)
		e = hq.SaveReplay(*pathRep)
		noError(e)
	}
}
