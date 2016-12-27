package main

import (
	"flag"
	"fmt"

	"bin/dctcp/rc"
	"react/sim/config"
	"react/sim/density"
	"react/sim/simlog"
)

var (
	flog = flag.String("log", "win.log", "log file to read")
	frc  = flag.String("rc", "sim.rc", "config file")
)

func main() {
	flag.Parse()
	rc.LoadRC(*frc)
	// rc.PrintRC()

	c := rc.TheRC()

	config.SetNhost(c.Nhost)
	config.WeekLen = 3000
	config.MinDayLen = 40
	config.TickPerGrid = 1
	config.LinkBw = c.LinkBw * c.UnitScaler * c.LinkScaler
	config.PackBw = c.PackBw * c.UnitScaler * c.LinkScaler

	density.BigThres = 150000 * c.UnitScaler * c.LinkScaler

	reader := simlog.NewMatReader(*flog)

	var e simlog.MatEntry
	d := new(density.Density)
	// dmax := new(density.Density)

	fmt.Printf("nze be neph bph nzph50 nzph90 bph50 bph90\n")
	for reader.Read(&e) {
		d.Count(e.M)
		// dmax.Max(d)

		// fmt.Println(d)
		fmt.Printf("%d %d %d %d %d %d %d %d\n",
			d.NonZeroElements, d.BigElements,
			d.NonZeroPerHostMax, d.BigPerHostMax,
			d.NonZeroPerHost50, d.NonZeroPerHost90,
			d.BigPerHost50, d.BigPerHost90,
		)
	}

	// fmt.Println(dmax)
}
