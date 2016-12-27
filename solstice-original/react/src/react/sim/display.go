package sim

import (
	. "react/sim/structs"
	"time"
)

type Display interface {
	Start(setup *Setup)

	TakesHour() bool
	DrawHour(r *HourReport)

	Stop()
}

type HourReport struct {
	T        uint64
	NewWeek  bool
	Send     Matrix
	Recv     Matrix
	CircRecv Matrix
	PackRecv Matrix
	Drop     Matrix
	Sched    Matrix
}

type Setup struct {
	Nhost     int
	LinkBw    uint64
	PackBw    uint64
	WeekLen   uint64
	MinDayLen uint64
	AvgDayLen uint64
	NightLen  uint64

	NicBufSize    uint64
	SwitchBufSize uint64

	TickPerHour uint64

	TickTime time.Duration
}
