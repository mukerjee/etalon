package config

import (
	"time"
)

var (
	Nhost int = 8
	Nlane int = 8 * 8

	LinkBw uint64 = 1250
	PackBw uint64 = 125

	WeekLen   uint64 = 1000
	MinDayLen uint64 = 100
	AvgDayLen uint64 = 200
	NightLen  uint64 = 20

	NicBufCover      uint64 = 4000
	NicBufTotalCover uint64 = 6000
	SwitchBufCover   uint64 = 1000

	TickPerHour uint64 = 100
	TickPerGrid uint64 = 4

	TickTime = time.Microsecond

	Tracking bool = true
)

func SetNhost(n int) {
	Nhost = n
	Nlane = n * n
}

func NicBufSize() uint64      { return LinkBw * NicBufCover }
func NicBufTotalSize() uint64 { return LinkBw * NicBufTotalCover }
func SwitchBufSize() uint64   { return LinkBw * SwitchBufCover }
