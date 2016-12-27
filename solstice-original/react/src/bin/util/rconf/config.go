package main

import (
	"strings"
	"time"
)

type Config struct {
	masterLocation string
	slaveLocation  string
	txLanes        []string
	rxLanes        []string

	timings *Timings
	flags   *Flags
	addrs   []MacAddr

	debugSelect uint64

	// TODO: queue mapping on each endhost
}

func NewConfig() *Config {
	ret := new(Config)

	ret.masterLocation = DefaultMasterLocation // master location is fixed
	assert(isValidLocation(ret.masterLocation))

	ret.SetSlaveLocation(DefaultSlaveLocation)

	ret.TxWithLanes(DefaultTxLanes)
	ret.RxWithLanes(DefaultRxLanes)

	ret.TimeForMindspd()
	ret.flags = new(Flags)
	ret.Watch("b3", "rx")

	ret.addrs = make([]MacAddr, nhost)
	for i := range ret.addrs {
		ret.addrs[i] = emptyMac
	}

	return ret
}

func parseLanes(lanes string) []string {
	ret := strings.Fields(lanes)
	assert(len(ret) == nhost)
	for _, lane := range ret {
		assert(isValidLane(lane))
	}

	return ret
}

func (self *Config) TimeForMindspd() {
	self.timings = MindspdTimings()
}

func (self *Config) TimeForMordia() {
	self.timings = MordiaTimings()
}

func (self *Config) SunsetAt(d time.Duration) {
	self.timings.ToNight = cycles(d)
}

func (self *Config) SyncSlaveAt(d time.Duration) {
	self.timings.ToSync = cycles(d)
}

func (self *Config) SwitchAt(d time.Duration) {
	self.timings.ToSwitch = cycles(d)
}

func (self *Config) PlanAt(d time.Duration) {
	self.timings.ToPlan = cycles(d)
}

func (self *Config) ConfigSwitchAt(d time.Duration) {
	self.timings.ToMap = cycles(d)
}

func (self *Config) TxWithLanes(lanes string) {
	self.txLanes = parseLanes(lanes)
}

func (self *Config) RxWithLanes(lanes string) {
	self.rxLanes = parseLanes(lanes)
}

func (self *Config) SetSlaveLocation(slave string) {
	assert(isValidLocation(slave))
	self.slaveLocation = slave
}

func (self *Config) SendPFC(b bool) {
	self.flags.SendPFC = b
}

func (self *Config) SendWeekend(b bool) {
	self.flags.SendWeekend = b
}

func (self *Config) SendWeekSignal(b bool) {
	self.flags.SendWeekSignal = b
}

func (self *Config) SyncInternally() {
	self.flags.SyncInternal = true
}

func (self *Config) SyncExternally() {
	self.flags.SyncInternal = false
}

func (self *Config) posFromIndex(i int) string {
	if i < 4 {
		return self.masterLocation
	}
	return self.slaveLocation
}

func (self *Config) Watch(lane, direction string) {
	assert(len(lane) == 2)
	ret := uint64(0)

	switch lane[0] {
	case 'a':
		ret += 0xa0
	case 'b':
		ret += 0xb0
	case 'c':
		ret += 0xc0
	case 'f':
		ret += 0xf0
	default:
		assert(false)
	}

	assert('0' <= lane[1] && lane[1] <= '3')
	ret += uint64(lane[1] - '0')
	if direction == "tx" {
		ret += 0x4
	} else {
		assert(direction == "rx")
	}

	self.debugSelect = ret
}

func (self *Config) txPort(i int) uint8 {
	lane := self.txLanes[i]
	pos := self.posFromIndex(i)
	return inPorts[pos][lane]
}

func (self *Config) rxPort(i int) uint8 {
	lane := self.txLanes[i]
	pos := self.posFromIndex(i)
	return outPorts[pos][lane]
}

func (self *Config) Apply(control *Control) {
	control.Reset()
	time.Sleep(time.Second / 2)

	self.ApplyLanes(control)
	self.ApplyMisc(control)
}

func (self *Config) ApplyLanes(control *Control) {
	// TODO:

	control.Reset()
	time.Sleep(time.Second / 2)
}

func (self *Config) ApplyMisc(control *Control) {
	control.SwitchInit()
	control.SetMacAddrs(self.addrs)
	control.SetFlags(self.flags)
	control.SetTimings(self.timings)
	control.SelectDebug(self.debugSelect)

	control.ResetSched()
}
