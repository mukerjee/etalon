package main

import (
	"flag"
	"fmt"
	"os"

	"bin/dctcp/rc"
	"react/sim/bvn"
	"react/sim/config"
	"react/sim/density"
	"react/sim/simlog"
	. "react/sim/structs"
)

var (
	flog   = flag.String("log", "win.log", "log file to read")
	frc    = flag.String("rc", "sim.rc", "config file")
	nhost  = flag.Int("n", 64, "nunber of hosts")
	minDay = flag.Uint64("d", 50, "min day length")
	packBw = flag.Uint64("p", 125, "packet bandwidth")
	prog   = flag.Bool("prog", false, "print progress to stderr")
)

type Brief struct {
	Circ       uint64
	CircQueued uint64
	Pack       uint64
}

func (self *Brief) Clear() {
	self.Circ = 0
	self.CircQueued = 0
	self.Pack = 0
}

func countBytes(d Matrix, days []*Day, bandw Matrix, ret *Brief) {
	ret.Clear()

	served := NewMatrix()
	t := NewMatrix()

	for _, day := range days {
		if day.Len <= config.NightLen {
			continue
		}

		copy(t, day.Sched)
		t.Mul(day.Len - config.NightLen)
		t.Mmul(bandw)
		served.Madd(t)
	}

	for i, b := range served {
		if b == 0 { // packet lane
			ret.Pack += d[i]
		} else if b < d[i] {
			ret.Circ += b
			ret.CircQueued += d[i] - b
		} else if d[i] > 0 {
			ret.Circ += d[i]
		}
	}
}

func norm(n uint64) float64 {
	return float64(n) / float64(config.Nhost) /
		float64(config.WeekLen) / float64(config.LinkBw) * 100
}

func main() {
	flag.Parse()
	rc.LoadRC(*frc)
	c := rc.TheRC()

	config.SetNhost(c.Nhost)
	config.WeekLen = 3000
	config.TickPerGrid = 1
	config.LinkBw = c.LinkBw * c.UnitScaler * c.LinkScaler
	config.PackBw = c.PackBw * c.UnitScaler * c.LinkScaler
	config.NightLen = 20
	config.MinDayLen = 40
	config.AvgDayLen = 200

	density.BigThres = 150000 * c.UnitScaler * c.LinkScaler

	reader := simlog.NewMatReader(*flog)

	var e simlog.MatEntry
	s := bvn.NewScheduler()
	s.DoInterleave = false

	fmt.Println("nday dem circ circq pack")
	var brief Brief
	nweek := 0
	for reader.Read(&e) {
		if *prog {
			fmt.Fprintf(os.Stderr, "%d\n", nweek)
		}
		nweek++

		demand := e.M
		nbytes := demand.Sum()
		days, bandw := s.Schedule(nil, demand, config.WeekLen)
		nday := len(days)
		countBytes(demand, days, bandw, &brief)
		fmt.Printf("%d %.3f %.3f %.3f %.3f\n",
			nday,
			norm(nbytes),
			norm(brief.Circ),
			norm(brief.CircQueued),
			norm(brief.Pack),
		)
	}
}
