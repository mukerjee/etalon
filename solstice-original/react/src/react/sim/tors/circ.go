package tors

import (
	. "react/sim"
	. "react/sim/blocks"
	. "react/sim/config"
	. "react/sim/packet"
	. "react/sim/queues"
	. "react/sim/structs"
)

type CircuitSwitch struct {
	nics *Queues

	NicQueue Matrix
	CircLink Matrix

	scheduler     Scheduler
	schedRecorder *SchedRecorder
	Calendar      *Calendar
}

var _ Switch = new(CircuitSwitch)

func NewCircuitSwitch() *CircuitSwitch {
	ret := new(CircuitSwitch)

	ret.nics = NewNics()
	ret.Calendar = NewCalendar()

	// NicQueue <= nics
	// Schedule <= Calendar
	ret.CircLink = NewMatrix()

	return ret
}

func (self *CircuitSwitch) Send(packet *Packet) uint64 {
	return self.nics.Send(packet)
}

func (s *CircuitSwitch) Tick(sink Block, e Estimator) (Matrix, Events) {
	assert(s.scheduler != nil)
	s.NicQueue = s.nics.Len()
	sched := s.Calendar.Plan(s.scheduler, e, s.NicQueue)
	if sched != nil && s.schedRecorder != nil {
		s.schedRecorder.Log(sched.Days)
	}

	curSched := s.Calendar.CurSched()
	s.nics.Move(curSched, sink, Destination, s.CircLink)

	s.Calendar.Tick()

	return curSched, s.Calendar.Events()
}

func (s *CircuitSwitch) Tdma() bool { return true }

func (s *CircuitSwitch) Bind(sc Scheduler, recorder *SchedRecorder) {
	s.scheduler = sc
	s.schedRecorder = recorder
}

func (s *CircuitSwitch) Served() (circ Matrix, pack Matrix) {
	return s.CircLink, nil
}
