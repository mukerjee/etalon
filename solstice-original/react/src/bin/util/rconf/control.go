package main

import (
	"time"
)

var (
	ethType = []byte{0x08, 0x00}
)

const (
	routeCtrl   = 0xfc
	routeReset  = 0xe1
	routeSwitch = 0xe2
)

const (
	cmdSchedSet  = 0x80
	cmdSchedSwap = 0x81

	cmdResetSched = 0x90
	cmdAddr       = 0x91
	cmdFlags      = 0x92
	cmdWeekSig    = 0x93
	cmdTimings    = 0x94
	cmdLaneSelect = 0x95
	cmdSenderConf = 0x9f

	cmdDebugSelect = 0xdb

	evWeekSig = 0x1e
)

type Control struct {
	comm Comm
	p    *Packer
}

func NewControl(comm Comm) *Control {
	ret := new(Control)
	ret.comm = comm
	ret.p = NewPacker()

	return ret
}

var testControl = NewControl(new(TestComm))

func (self *Control) start(route uint8) {
	self.startExtra(route, 0)
}

func (self *Control) startExtra(route, extra uint8) {
	p := self.p

	p.clear()
	p.put(broadcastMac)
	p.c(route)
	p.c(extra)
	p.pad(macAddrSize - 2)
	p.put(ethType)
}

func (self *Control) startCtrl(cmd uint8) {
	self.startCtrlExtra(cmd, 0)
}

func (self *Control) startCtrlExtra(cmd, extra uint8) {
	self.start(routeCtrl)

	self.p.c(cmd)
	self.p.c(extra)
}

func (self *Control) send() {
	self.p.sendVia(self.comm)
}

func (self *Control) Reset() {
	self.start(routeReset)
	self.send()
}

func (self *Control) ResetSched() {
	self.startCtrl(cmdResetSched)

	self.send()
}

func (self *Control) SetMacAddrs(addrs []MacAddr) {
	assert(len(addrs) <= nhost)

	for i := 0; i < nhost; i++ {
		if i < len(addrs) {
			self.p.put(addrs[i])
		} else {
			self.p.put(emptyMac)
		}
	}
}

func (self *Control) SetFlags(flags *Flags) {
	self.startCtrl(cmdFlags)
	self.p.c(flags.Byte())
	self.send()
}

func (self *Control) SetWeekSignalPosition(p uint64) {
	self.startCtrl(cmdWeekSig)
	self.p.u64(p)
	self.send()
}

func (self *Control) SetTimings(timings *Timings) {
	self.startCtrl(cmdTimings)
	timings.packTo(self.p)
	self.send()
}

func (self *Control) SelectDebug(lane uint64) {
	self.startCtrl(cmdDebugSelect)
	self.p.u64(lane)
	self.send()
}

func (self *Control) SwitchWrite(addr uint16, bytes []byte) {
	n := len(bytes)
	assert(n <= 48)
	self.startExtra(routeSwitch, uint8(n))
	self.p.c(uint8(addr >> 8))
	self.p.c(uint8(addr))
	self.p.put(bytes)
	self.send()

	time.Sleep(time.Millisecond * 10)
}

func (self *Control) SwitchWriteByte(addr uint16, b byte) {
	bytes := []byte{b}
	assert(len(bytes) == 1)
	self.SwitchWrite(addr, bytes)
}

func (self *Control) switchInitCommon() {
	self.SwitchWriteByte(0x000f, 0x10)
	self.SwitchWriteByte(0x0009, 0xf0)
	self.SwitchWriteByte(0x000b, 0x82)
	self.SwitchWriteByte(0x000a, 0x00)
}

func (self *Control) SwitchInit() {
	self.switchInitCommon()
	self.SwitchWriteByte(0x0003, 0x98)
}

func (self *Control) SwitchInitStatic() {
	self.switchInitCommon()
	self.SwitchWriteByte(0x0003, 0x88)
}

func (self *Control) Setup(config *Config) {
	config.Apply(self)
}
