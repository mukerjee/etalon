package sim

import (
	"bytes"
	"errors"
	"io"
	"reflect"
	"runtime"

	"react/sim/clock"
	. "react/sim/config"
	. "react/sim/structs"
)

// A Testbed runs a simulation
type Testbed struct {
	Hosts     Hosts     // Send and receive packets
	Switch    Switch    // Transmits packets from host to host
	Estimator Estimator // (TDMA switch only), estimates the upcoming demands
	Scheduler Scheduler // (TDMA switch only), breaks the demand into days
	Monitor   Monitor   // (optional) tracks past demands
	Stopper   Stopper   // (optional) tells the testbed to stop
	Display   Display   // (optional) displays the simulation status
	Progress  Progress  // (optional) shows the simulation progress

	recorder      *Recorder      // recorder
	schedRecorder *SchedRecorder // schedule logger

	// these two binds the hosts and the switch
	// which also provids the current demand
	// and throughput stats (goodput and bytes dropped)
	exit        *exit
	entrance    *entrance
	seenNewWeek bool
	events      Events
	sched       *Counter

	WarmUp  uint64
	EndTime uint64 // The time that the simulation ends

	*ProgressStat
}

func NewTestbed(h Hosts, s Switch) *Testbed {
	ret := new(Testbed)
	ret.Hosts = h
	ret.Switch = s

	ret.exit = newExit(h)
	ret.entrance = newEntrance(s)
	ret.sched = NewCounter()
	ret.ProgressStat = new(ProgressStat)

	return ret
}

func (self *Testbed) tick() {
	self.entrance.Clear()

	// the entrance will relay the packets to the switch
	self.Hosts.Tick(self.entrance)

	// and the exit will relay the packets to the hosts
	s, ev := self.Switch.Tick(self.exit, self.Estimator)
	if self.Switch.Tdma() {
		self.sched.CountMatrix(s)
		self.seenNewWeek = self.seenNewWeek || ev.Test(NewWeek)
	}
	self.events = ev

	circ, pack := self.Switch.Served()
	self.exit.CountServed(circ, pack)

	if self.Monitor != nil {
		self.Monitor.Tell(self.entrance.send.Cur)
	}
}

func queryMatrix(i interface{}, name string) Matrix {
	v := reflect.ValueOf(i).Elem()
	f := v.FieldByName(name)
	if !f.IsValid() {
		return nil
	}
	ret, okay := f.Interface().(Matrix)
	if okay {
		return ret
	}
	return nil
}

func (self *Testbed) query(name string) (ret Matrix) {
	ret = queryMatrix(self, name)
	if ret != nil {
		return ret
	}
	ret = queryMatrix(self.Switch, name)
	if ret != nil {
		return ret
	}
	ret = queryMatrix(self.Hosts, name)
	if ret != nil {
		return ret
	}
	return nil
}

func (self *Testbed) check() error {
	if self.Hosts == nil {
		return errors.New("Missing Hosts")
	}

	if self.Switch == nil {
		return errors.New("Missing Switch")
	}

	if self.Estimator == nil && self.Switch.Tdma() {
		return errors.New("Missing Estimator")
	}

	if self.Scheduler == nil && self.Switch.Tdma() {
		return errors.New("Missing Scheduler")
	}

	if self.Switch.Tdma() {
		self.Switch.Bind(self.Scheduler, self.schedRecorder)
	}

	return nil
}

func makeSetup() *Setup {
	return &Setup{
		Nhost:         Nhost,
		LinkBw:        LinkBw,
		PackBw:        PackBw,
		WeekLen:       WeekLen,
		MinDayLen:     MinDayLen,
		AvgDayLen:     AvgDayLen,
		NightLen:      NightLen,
		NicBufSize:    NicBufSize(),
		SwitchBufSize: SwitchBufSize(),
		TickPerHour:   TickPerHour,
		TickTime:      TickTime,
	}
}

func (self *Testbed) _draw() {
	// an hour just passed
	report := &HourReport{
		T:        clock.T,
		NewWeek:  self.seenNewWeek,
		Send:     self.entrance.send.Sum,
		Recv:     self.exit.recv.Sum,
		CircRecv: self.exit.circ.Sum,
		PackRecv: self.exit.pack.Sum,
		Drop:     self.exit.drop.Sum,
		Sched:    self.sched.Sum,
	}

	self.Display.DrawHour(report)
}

func (self *Testbed) clearSum() {
	self.entrance.ClearSum()
	self.exit.ClearSum()
	self.sched.ClearSum()
	self.seenNewWeek = false
}

func (self *Testbed) progress() {
	self.T = clock.T
	self.Goodput = self.exit.Goodput
	self.Circput = self.exit.Circput
	self.Packput = self.exit.Packput
	self.Dropped = self.exit.Dropped
	if self.T >= self.WarmUp {
		self.Capacity += uint64(Nhost) * LinkBw
	}

	if self.Progress == nil {
		return
	}

	self.Progress.Tick(self.ProgressStat)
}

func (self *Testbed) draw() {
	if (clock.T+1)%TickPerHour != 0 {
		return
	}

	defer self.clearSum()

	if self.Display == nil {
		return
	}
	if !self.Display.TakesHour() {
		return
	}

	self._draw()
	self.clearSum()
}

func (self *Testbed) shouldStop(t uint64) bool {
	if clock.T >= t {
		return true
	}
	if self.Stopper != nil && self.Stopper.ShouldStop() {
		return true
	}

	return false
}

func (self *Testbed) clear() {
	self.entrance.Clear()
	self.exit.Clear()
	self.sched.Clear()
	self.events = Noop
}

func (self *Testbed) record() {
	defer self.clear()

	if self.recorder == nil {
		return
	}

	tick := &Tick{
		T: clock.T,

		NewWeek: self.events.Test(NewWeek),
		Dusk:    self.events.Test(Dusk),
		Dawn:    self.events.Test(Dawn),

		Send:  self.entrance.send.Cur,
		Recv:  self.exit.recv.Cur,
		Circ:  self.exit.circ.Cur,
		Pack:  self.exit.pack.Cur,
		Drop:  self.exit.drop.Cur,
		Sched: self.sched.Cur,
	}
	self.recorder.Record(tick)
}

func (self *Testbed) Run(t uint64) error {
	self.EndTime = 0
	self.Goodput = 0
	self.exit.Starts = self.WarmUp

	if err := self.check(); err != nil {
		return err
	}

	assert(t >= 0)

	clock.Reset()
	if self.Progress != nil {
		self.Progress.Start(self.ProgressStat)
	}
	if self.Display != nil {
		self.Display.Start(makeSetup())
	}
	for {
		self.tick()
		self.progress()
		self.draw() // will serve as back pressure
		self.record()

		if self.shouldStop(t) {
			break
		}

		clock.Tick()

		runtime.Gosched()
	}

	if self.Display != nil {
		self.Display.Stop()
	}
	if self.Progress != nil {
		self.Progress.Stop(self.ProgressStat)
	}

	self.EndTime = clock.T // copy the time out
	self.Goodput = self.exit.Goodput
	return nil
}

func (self *Testbed) RecordTo(w io.Writer) {
	self.recorder = NewRecorder(w)
}

func (self *Testbed) LogSchedTo(w io.Writer) {
	self.schedRecorder = NewSchedRecorder(w)
}

func titleString(name string) string {
	buf := new(bytes.Buffer)

	for i, c := range name {
		switch {
		case 'A' <= c && c <= 'Z':
			if i > 0 {
				buf.WriteRune(' ')
			}
			buf.WriteRune(c + 'a' - 'A')
		case c == '_':
			buf.WriteRune(' ')
		default:
			buf.WriteRune(c)
		}
	}
	return buf.String()
}

func (self *Testbed) Summarize(out io.Writer) error {
	return self.exit.Summarize(out)
}
