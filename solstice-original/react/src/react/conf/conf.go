package conf

import (
	"react/testb/setup"
)

type Config struct {
	Hosts     []int
	Demand    string
	Estimator string

	Sched   Sched
	DemGrow DemGrow
	DemCali DemCali
	Route   Route
}

type Sched struct {
	Diag        bool
	CleanOnIdle bool
	Shuffle     bool
	Interleave  bool
	SafeBandw   bool
	FullBandw   bool
	PureCircuit bool
	SkipPrefind bool
	UseC        bool
}

type Route struct {
	AllPacket bool
}

type DemGrow struct {
	Share   int
	BigOnly bool
}

type DemCali struct {
	Nbig   int
	Nsmall int
}

var Conf Config

func init() {
	Conf.Hosts = setup.FullTestbed()
	Conf.Demand = "all2all"
	Conf.Estimator = "future"

	Conf.Sched.Shuffle = false
	Conf.Sched.Interleave = true
	Conf.Sched.SafeBandw = true
	Conf.Sched.SkipPrefind = false

	Conf.DemGrow.Share = 3
	Conf.DemGrow.BigOnly = false

	Conf.DemCali.Nbig = 1
	Conf.DemCali.Nsmall = 3

	Conf.Route.AllPacket = false
}
