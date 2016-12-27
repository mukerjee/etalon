package tors

import (
	. "react/sim"
	. "react/sim/blocks"
	. "react/sim/config"
	. "react/sim/drainer"
	. "react/sim/packet"
	"react/sim/pqueues"
	. "react/sim/queues"
	. "react/sim/structs"

	// "fmt"
	// "react/sim/clock"
)

type ReactSwitch struct {
	nics       ReactNics
	nics2      ReactNics
	packSwitch *Queues
	downRaters *Queues

	NicQueue Matrix
	Schedule Matrix
	CircLink Matrix
	PackUp   Matrix
	PackUp2  Matrix
	PackBuf  Matrix
	PackDown Matrix
	DownThru Matrix

	PackBufDrop Matrix
	DownDrop    Matrix

	DemandEst Matrix

	packUpLimits  []*Limit
	packUpLimits2 []*Limit
	rowBuf        Vector
	rowBuf2       Vector

	packUpDrainer   *Drainer
	packUpDrainer2  *Drainer
	packDownDrainer *LinkDrainer
	downDrainer     *LinkDrainer

	scheduler     Scheduler
	schedRecorder *SchedRecorder
	Calendar      *Calendar
}

var _ Switch = new(ReactSwitch)

type ReactNics interface {
	Send(p *Packet) uint64
	Len() Matrix
	Move(size Matrix, next Block, loc int, ret Matrix)
	MoveUsing(next Block, loc int, puller Puller) (buf, pulled Matrix)
}

func NewReactSwitch() *ReactSwitch {
	return newReactSwitch(false, false)
}

func NewPreactSwitch() *ReactSwitch {
	return newReactSwitch(true, false)
}

func NewDreactSwitch() *ReactSwitch {
	return newReactSwitch(false, true)
}

func newReactSwitch(preact bool, twoQueues bool) *ReactSwitch {
	ret := new(ReactSwitch)

	if !preact {
		ret.nics = NewNics()
	} else {
		ret.nics = pqueues.NewQueues()
	}
	if twoQueues {
		ret.nics2 = NewNics()
	}

	ret.packSwitch = NewSizedQueues(SwitchBufSize())
	ret.downRaters = NewQueues()

	ret.rowBuf = NewVector()
	ret.rowBuf2 = NewVector()

	limits := make([]*Limit, 0, Nhost+1)
	limits = RowLimits(limits, PackBw)
	limits = append(limits, NewLimit(make([]int, 0, Nlane), 0))

	limits2 := make([]*Limit, 0, Nhost+1)
	limits2 = RowLimits(limits2, PackBw)

	ret.packUpLimits = limits
	ret.packUpDrainer = NewDrainer(limits)

	ret.packUpLimits2 = limits2
	ret.packUpDrainer2 = NewDrainer(limits2)

	ret.packDownDrainer = NewDownlinkDrainer(PackBw)

	ret.downDrainer = NewDownlinkDrainer(LinkBw)

	// NicQueue <= nics
	// Schedule <= Calendar
	ret.CircLink = NewMatrix()
	// PackUp <= packUpDrainer
	// PackBuf <= packSwitch
	// PackDown <= packDownDrainer
	// DownThru <= downDrainer
	ret.DownDrop = NewMatrix()
	ret.PackBufDrop = NewMatrix()

	ret.Calendar = NewCalendar()

	return ret
}

func (self *ReactSwitch) Send(packet *Packet) uint64 {
	if packet.Hint != Mouse {
		// Unknown or Elephant
		// just go with the normal nic
		return self.nics.Send(packet)
	}

	// no second nic, then go to the normal nic
	if self.nics2 == nil {
		return self.nics.Send(packet)
	}

	// we have two queues
	// and this packet belongs to a mouse flow
	// so go via the "car-pool" fast lane.
	return self.nics2.Send(packet)
}

func boundUpLimits(limits []*Limit, bandw uint64, left Vector) {
	for i := 0; i < Nhost; i++ {
		bw := bandw
		if left[i] < bw {
			bw = left[i]
		}
		limits[i].Bound = bw
	}
}

func (self *ReactSwitch) setPackLanes(circBandw Matrix) {
	assert(circBandw != nil)
	circLimit := self.packUpLimits[Nhost]
	circLanes := circLimit.Lanes[0:0]

	for i := 0; i < Nhost; i++ {
		limit := self.packUpLimits[i]
		lanes := limit.Lanes[0:0]

		for j := 0; j < Nhost; j++ {
			lane := i*Nhost + j
			if circBandw[lane] == 0 {
				lanes = append(lanes, lane)
			} else {
				circLanes = append(circLanes, lane)
			}
		}

		limit.Lanes = lanes
	}

	circLimit.Lanes = circLanes
}

func (s *ReactSwitch) setDownDrainerBounds() {
	for dest := range s.downDrainer.Bounds {
		circUsed := uint64(0)
		for src := 0; src < Nhost; src++ {
			circUsed += s.CircLink[Lane(src, dest)]
		}
		assert(circUsed <= LinkBw)
		s.downDrainer.Bounds[dest] = LinkBw - circUsed
	}
}

func (s *ReactSwitch) Tick(sink Block, e Estimator) (Matrix, Events) {
	assert(e != nil)

	s.NicQueue = s.nics.Len()
	sched := s.Calendar.Plan(s.scheduler, e, s.NicQueue)
	if sched != nil {
		// fmt.Println(sched)
		// a new schedule just applied
		s.setPackLanes(sched.Bandw)
		if s.schedRecorder != nil {
			s.schedRecorder.Log(sched.Days)
		}
	}

	// circ link
	s.Schedule = s.Calendar.CurSched()
	// s.nics.Move(s.Schedule, s.downRaters, DownMerger, s.CircLink)
	s.nics.Move(s.Schedule, sink, Destination, s.CircLink)
	s.NicQueue = s.nics.Len()

	if s.nics2 != nil {
		s.CircLink.RowSum(s.rowBuf2)
		assert(s.rowBuf2.Leq(LinkBw))
		s.rowBuf2.SubBy(LinkBw)
		boundUpLimits(s.packUpLimits2, PackBw, s.rowBuf2)
		_, s.PackUp2 = s.nics2.MoveUsing(s.packSwitch,
			SwitchBuffer, s.packUpDrainer2)
		s.PackUp2.RowSum(s.rowBuf2)
	} else {
		// s.rowBuf2.Clear()
	}
	// s.rowBuf2 now saves the link bandwidth used by nics2

	// pack uplink
	s.CircLink.RowSum(s.rowBuf)
	s.rowBuf.SubBy(LinkBw) // now it holds
	s.rowBuf.Cap(PackBw)
	s.rowBuf.Vtrim(s.rowBuf2)
	assert(s.rowBuf.Leq(PackBw))
	boundUpLimits(s.packUpLimits, PackBw, s.rowBuf)

	_, s.PackUp = s.nics.MoveUsing(s.packSwitch,
		SwitchBuffer, s.packUpDrainer)

	// downlink step 1: out of the packet switch
	s.PackBuf, s.PackDown = s.packSwitch.MoveUsing(s.downRaters,
		DownMerger, s.packDownDrainer)

	/*
		fmt.Printf("t=%d up=%d up2=%d buf=%d down=%d\n", clock.T,
			s.PackUp[50], s.PackUp2[50], s.PackBuf[50], s.PackDown[50],
		)
		for i := 0; i < Nhost; i++ {
			fmt.Printf("%4d ", s.PackDown[i*Nhost+18]/100)
		}
		fmt.Println()
		for i := 0; i < Nhost; i++ {
			fmt.Printf("%4d ", s.PackUp2[1*Nhost+i]/100)
		}
		fmt.Println()
		for i := 0; i < Nhost; i++ {
			fmt.Printf("%4d ", s.PackUp[1*Nhost+i]/100)
		}
		fmt.Println()
		fmt.Println()
	*/

	// downlink step 2: down rating
	s.setDownDrainerBounds()
	_, s.DownThru = s.downRaters.MoveUsing(sink,
		Destination, s.downDrainer)
	s.downRaters.DropAll(sink, Downlink, s.DownDrop)
	assert(s.PackDown.MnolessThan(s.DownDrop))
	s.PackDown.Msub(s.DownDrop)

	// packet buffer drop
	s.packSwitch.CutToCapacity(sink, PacketBuffer, s.PackBufDrop)

	s.Calendar.Tick()
	return s.Schedule, s.Calendar.Events()
}

func (s *ReactSwitch) Tdma() bool { return true }

func (s *ReactSwitch) Bind(sc Scheduler, recorder *SchedRecorder) {
	s.scheduler = sc
	s.schedRecorder = recorder
}

func (s *ReactSwitch) Served() (circ Matrix, pack Matrix) {
	return s.CircLink, s.PackDown
}
