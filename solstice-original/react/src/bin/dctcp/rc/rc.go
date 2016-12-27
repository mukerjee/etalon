package rc

import (
	"encoding/json"
	"fmt"
	"os"
)

type RC struct {
	Nhost    int
	WinLog   string
	FlowLog  string
	SchedLog string
	Summary  string
	Sched    string // Scheduler
	Workload string

	Tmax       uint64
	Switch     string
	PackBw     uint64
	LinkBw     uint64
	UnitScaler uint64
	LinkScaler uint64
	LoadScaler float64

	SmallFlowFreq float64

	SafeBandw bool

	NoFlows   bool
	NoQueries bool
	NoBig     bool
	NoSmall   bool
}

var rc *RC

func init() {
	rc = new(RC)

	rc.Nhost = 64
	rc.WinLog = "win.log"
	rc.FlowLog = "flows.log"
	rc.SchedLog = "sched.log"
	rc.Summary = "summary"
	rc.Sched = "sol"
	rc.Workload = "dctcp"
	rc.Tmax = 5e5
	rc.Switch = "pack"
	rc.LinkBw = 1250
	rc.PackBw = 125
	rc.SafeBandw = false
	rc.UnitScaler = 100
	rc.LinkScaler = 1
	rc.LoadScaler = 1

	rc.NoFlows = false
	rc.NoQueries = false
	rc.NoBig = false
	rc.NoSmall = false

	rc.SmallFlowFreq = 1
}

func noError(e error) {
	if e != nil {
		panic(e)
	}
}

func LoadRC(path string) {
	frc, e := os.Open(path)
	noError(e)

	noError(json.NewDecoder(frc).Decode(rc))
	noError(frc.Close())
}

func PrintRC() {
	bytes, e := json.MarshalIndent(rc, "", "  ")
	noError(e)
	fmt.Println(string(bytes))
}

func TheRC() *RC { return rc }
