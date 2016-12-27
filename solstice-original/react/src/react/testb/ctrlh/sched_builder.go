package ctrlh

import (
	"fmt"
	"io"
	"net"
	"time"

	"rawpack"
	"react/testb/setup"
)

type SchedBuilder struct {
	// Pause flags
	NoDayPausing   bool
	NoNightPausing bool
	NoPacketQueue  bool

	// Route flags
	AllPacket  bool
	AllCircuit bool

	// Circuit switch port mappings
	Ports *Ports

	// Endhost syncing flags
	NoSyncEndHosts bool
	NoSyncRemaps   bool
	NoSyncRates    bool

	NoRemap        bool // disable remap
	AllPacketRemap bool // map all queues to packet queue
	SafeRates      bool // map all circuit links to linkbw - packbw
	FullRates      bool

	NoLoadStart bool // if notify workload start on firt week

	// weeksig position in us, default(0) means 500us
	WeeksigPosition uint64

	// NoSafeEnd bool // if not add a round robin at the end

	Hosts []int
}

func (self *SchedBuilder) buildDay(d *SchedDay) *schedSlice {
	if setup.Nhost != 8 {
		panic("wrong nhost")
	}

	dests := mapDests(d)
	slice := newSchedSlice(d.Len)

	relSets(dests, slice.daywl)

	if !self.NoPacketQueue {
		setOnes(slice.daywl)
		setOnes(slice.nightwl)
	}

	if self.NoDayPausing {
		setAll(slice.daywl)
	}
	if self.NoNightPausing {
		setAll(slice.nightwl)
	}

	if self.AllCircuit {
		setAll(slice.circwl)
	} else if !self.AllPacket {
		absSets(dests, slice.circwl)
	}

	// fill the destinations, so that every port has a mapping
	fillDestsHosts(dests, self.Hosts)
	fillDests(dests)

	if self.Ports != nil {
		for src, dest := range dests {
			out := self.Ports.Output[dest]
			in := self.Ports.Input[src]
			slice.portMap[out] = in
		}
	}

	return slice
}

const slicePerPacket = 16

func boolByte(b bool) byte {
	if b {
		return 1
	}
	return 0
}

func routeSrcMac(id int) net.HardwareAddr {
	ret := net.HardwareAddr(make([]byte, len(BroadcastMAC)))
	copy(ret, BroadcastMAC)
	ret[0] = routeEndHost(id)

	return ret
}

func (self *SchedBuilder) buildUpdate(s *SchedWeek, id int, c int) *Packet {
	update := newEndhostUpdate(uint32(s.Index))

	update.remapValid = boolByte(!self.NoSyncRemaps)
	update.doRemap = boolByte(!self.NoRemap)

	if !(self.NoSyncRemaps || self.NoRemap || self.AllPacketRemap) {
		for _, d := range s.Days {
			for _, lane := range d.Lanes {
				if lane.Src != id {
					continue
				}

				qid := lane.Qid()
				update.remaps[qid] = uint8(qid)
			}
		}
	}

	update.rateValid = boolByte(!self.NoSyncRates)
	if self.SafeRates {
		update.rates[0] = setup.PackRatio * 8 / 10
		for i := 1; i < setup.Nhost; i++ {
			update.rates[i] = setup.CircRatio
		}
	} else if self.FullRates {
		for i := 0; i < setup.Nhost; i++ {
			update.rates[i] = 100
		}
	} else {
		// TODO: we need to think more about this...
		update.rates[0] = byte(100 - c)
		for i := 1; i < setup.Nhost; i++ {
			update.rates[i] = byte(c)
		}
	}

	udp := &rawpack.UDPPacket{
		SrcMAC:  routeSrcMac(id),
		SrcIP:   rawpack.IP(setup.CtrlIP),
		SrcPort: setup.CtrlPort,

		DestMAC:  BroadcastMAC,
		DestIP:   rawpack.IP(setup.BroadcastIP),
		DestPort: setup.CtrlPort,
	}

	p := NewPacket()
	p.Bytes(udp.MakeHeader(update.Len()))
	update.writeTo(p)

	return p
}

func (self *SchedBuilder) Build(s *SchedWeek) []*Packet {
	if len(s.Days) > 128 {
		panic("too many days in a week")
	}

	ret := make([]*Packet, 0, setup.Nhost*3)

	var p *Packet
	for i, d := range s.Days {
		if i%slicePerPacket == 0 {
			p = NewCtrlPacketExtra(cmdSchedSet, byte(i))
			ret = append(ret, p)
		}

		slice := self.buildDay(d)
		slice.writeTo(p)
	}

	// packet to end hosts
	if !self.NoSyncEndHosts {
		minRate := s.MinRate()
		if minRate < setup.CircRatio {
			minRate = setup.CircRatio
		}

		for i := 0; i < setup.Nhost; i++ {
			p = self.buildUpdate(s, i, minRate)
			ret = append(ret, p)
		}
	}

	weeksigPosition := self.WeeksigPosition
	if weeksigPosition == 0 {
		weeksigPosition = 500
	}
	weekLen := s.Len()
	if weekLen <= weeksigPosition {
		panic("week too short")
	}
	p = NewWeeksigPosition(weekLen - weeksigPosition)
	ret = append(ret, p)

	// commit packet
	p = NewSchedSwap(len(s.Days), s.Index == 0 && !self.NoLoadStart)
	ret = append(ret, p)

	return ret
}

func (self *SchedBuilder) CompileWeek(w io.Writer, s *SchedWeek) error {
	packs := self.Build(s)
	var e error
	for _, p := range packs {
		e = CtrlSendOnSig(w, p)
		if e != nil {
			return e
		}
	}

	return CtrlWaitSig(w, uint16(len(s.Days)))
}

func (self *SchedBuilder) Compile(w io.Writer, s *Schedule) error {
	for _, week := range s.Weeks {
		e := self.CompileWeek(w, week)
		if e != nil {
			return e
		}
	}

	return self.CompileWeek(w, DefaultWeek(len(s.Weeks)))
}

func (self *SchedBuilder) CompileTCP(addr string, s *Schedule) error {
	a := fmt.Sprintf("%s:%d", addr, setup.CtrlRtPort)
	_conn, e := net.DialTimeout("tcp4", a, time.Second)
	if e != nil {
		return e
	}
	conn := _conn.(*net.TCPConn)

	defer conn.Close()

	return self.Compile(conn, s)
}
