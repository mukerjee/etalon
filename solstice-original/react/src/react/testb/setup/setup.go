package setup

import (
	"fmt"

	"react/sim/config"
)

const (
	Nhost        = 8
	Nlane        = Nhost * Nhost
	LinkBw       = 1250
	PackBw       = 125
	CircBw       = LinkBw - PackBw
	PackRatio    = 100 * PackBw / LinkBw
	CircRatio    = 100 - PackRatio
	NanosPerTick = 1000

	LoadgPort  = 8201 // UDP
	BeatsPort  = 8202 // UDP
	CtrlPort   = 8203 // UDP
	CtrlRtPort = 8203 // TCP, runtime

	PackSize = 1400

	CtrlMAC     = "90e2.ba2f.8eac" // stands for reactor
	CtrlIP      = "192.168.1.40"
	CtrlHostIP  = "172.22.16.57"
	BroadcastIP = "192.168.1.255"

	hostFmt    = "reactor%d"
	ipFmt      = "192.168.1.5%d"
	localIpFmt = "172.22.16.5%d"
	localIface = "eth0"

	Nport = 48

	WeekLen   = 1500
	AvgDayLen = 300
	MinDayLen = 160
	NightLen  = 30
)

func FullTestbed() []int {
	ret := make([]int, Nhost)
	for i := range ret {
		ret[i] = i
	}
	return ret
}
func LittleHalf() []int {
	ret := FullTestbed()
	return ret[:Nhost/2]
}

func BigHalf() []int {
	ret := FullTestbed()
	return ret[Nhost/2:]
}

const (
	LoadgMagic   = 0x372153AE
	LoadgStart   = 0x372253AE
	WeekendMagic = 0x372353AE
	UpdateMagic  = 0x372453AE
)

func CheckId(id int) bool {
	return id >= 0 && id < Nhost
}

func LocalIp(id int) string {
	if !CheckId(id) {
		return ""
	}
	return fmt.Sprintf(localIpFmt, id)
}

func Ip(id int) string {
	if !CheckId(id) {
		return ""
	}
	return fmt.Sprintf(ipFmt, id)
}

func Host(id int) string {
	if !CheckId(id) {
		return ""
	}
	return fmt.Sprintf(hostFmt, id)
}

func LocalIface(id int) string {
	return localIface
}

func ConfigSim() {
	config.SetNhost(Nhost)

	config.LinkBw = LinkBw
	config.PackBw = PackBw

	config.WeekLen = WeekLen
	config.MinDayLen = MinDayLen
	config.AvgDayLen = AvgDayLen
	config.NightLen = NightLen
}
