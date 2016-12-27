package main

import (
	"flag"
	"fmt"

	"bin/dctcp/rc"
	"react/sim/config"
	"react/sim/simlog"
)

var (
	flog = flag.String("log", "win.log", "log file to read")
	frc  = flag.String("rc", "sim.rc", "config file")
)

func main() {
	flag.Parse()
	rc.LoadRC(*frc)

	c := rc.TheRC()

	config.SetNhost(c.Nhost)
	config.WeekLen = 3000
	config.MinDayLen = 40
	config.NightLen = 20
	config.TickPerGrid = 1
	config.LinkBw = c.LinkBw * c.UnitScaler * c.LinkScaler
	config.PackBw = c.PackBw * c.UnitScaler * c.LinkScaler

	reader := simlog.NewMatReader(*flog)
	norm := config.WeekLen * config.LinkBw

	var e simlog.MatEntry

	for reader.Read(&e) {
		m := e.M
		for i := 0; i < config.Nhost; i++ {
			for j := 0; j < config.Nhost; j++ {
				lane := i*config.Nhost + j
				num := int(float64(m[lane]) * 999 / float64(norm))
				if num == 0 {
					fmt.Printf("%3s ", "-")
				} else {
					fmt.Printf("%3d ", num)
				}
			}
			fmt.Printf("\n")
		}
		fmt.Printf("\n")
	}
}
