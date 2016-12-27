package main

import (
	"os"

	. "react/sim/config"
	"react/sim/pattern"
)

func main() {
	SetNhost(8)

	p1 := new(pattern.P1)

	p1.Name = "test"
	p1.N = 20e3
	p1.Bandw = 1000

	p1.Small = 50
	p1.SmallBandw = 30
	p1.SmallLen = 7e3 / p1.SmallBandw

	p1.Big = 7
	p1.BigPeriod = 5e3

	dem := p1.Make()
	dem.PrintTo(os.Stdout)
}
