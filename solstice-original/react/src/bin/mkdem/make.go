package main

import (
	"log"
	"react/conf"
	"react/demand"
	"react/testb/setup"
)

func makeDemand(name string, hosts []int) *demand.Demand {
	switch name {
	case "grow":
		return makeGrow(hosts)
	case "all2all":
		return makeAll2All(hosts)
	case "patsw":
		return makePatSwitch(hosts)
	case "cali":
		return makeCalibrate(hosts)
	}

	log.Fatalf("error: invalid demand: %s", name)
	return nil
}

func makeAll2All(hosts []int) *demand.Demand {
	p := &All2All{
		Nhost:   setup.Nhost,
		Hosts:   hosts,
		LinkBw:  setup.LinkBw * 91 / 100,
		T:       3e6,
		Twarmup: 3000,
	}

	return p.Make()
}

func makeGrow(hosts []int) *demand.Demand {
	share := conf.Conf.DemGrow.Share

	fillBw := uint64(setup.LinkBw * 93 / 100)
	smallBw := setup.PackBw * 7 / 10 / uint64(share)
	if conf.Conf.DemGrow.BigOnly {
		smallBw = 0
	}

	p := &Grow{
		Nhost:   setup.Nhost,
		Hosts:   hosts,
		Share:   share,
		Twarmup: 3000,
		Tstable: 6000,
		Trest:   9000,
		FillBw:  fillBw,
		SmallBw: smallBw,
		Repeat:  3,
	}

	return p.Make()
}

func makePatSwitch(hosts []int) *demand.Demand {
	p := &PatSwitch{
		Nhost:   setup.Nhost,
		Hosts:   hosts,
		LinkBw:  setup.LinkBw * 81 / 100,
		Twarmup: 3750,
		Tsep:    9000,
		Tcross:  9000,
		Repeat:  5,
	}

	return p.Make()
}

func makeCalibrate(hosts []int) *demand.Demand {
	p := &Calibrate{
		Nhost:   setup.Nhost,
		Nbig:    conf.Conf.DemCali.Nbig,
		Nsmall:  conf.Conf.DemCali.Nsmall,
		Hosts:   hosts,
		FillBw:  setup.LinkBw,
		SmallBw: 15,
		Twarmup: 3000,
		Tstable: 4500,
		Trest:   9000,
	}

	return p.Make()
}
