package tors

import (
	"bytes"
	"fmt"
	"log"

	. "react/sim"
	. "react/sim/config"
	. "react/sim/structs"
)

// A Calendar saves the schedule of the current week
// and provides the schedule matrix for each tick
type Calendar struct {
	idleDays []*Day
	days     []*Day

	zeros Matrix

	curDay    int
	timeOfDay uint64

	CurDay    int
	TimeOfDay uint64
	IsNight   bool
	IsDusk    bool
	Table     []*Day
}

func NewCalendar() *Calendar {
	ret := new(Calendar)

	ret.zeros = NewMatrix()

	return ret
}

// Returns the slice for today
func (self *Calendar) Today() *Day {
	return self.days[self.curDay]
}

// Returns true when the calendar runs to an end
func (self *Calendar) needSchedule() bool {
	return self.days == nil
}

func (self *Calendar) Events() Events {
	ev := Noop
	if self.TimeOfDay == 0 {
		ev = ev.Set(Dawn)
		if self.CurDay == 0 {
			ev = ev.Set(NewWeek)
		}
	} else if self.IsDusk {
		ev = ev.Set(Dusk)
	}

	return ev
}

func (self *Calendar) tick() {
	self.timeOfDay++
	if self.timeOfDay < self.Today().Len {
		return
	}

	// day reset
	self.timeOfDay = 0
	self.curDay++
	if self.curDay < len(self.days) {
		return
	}

	self.curDay = 0
	self.days = nil // need new schedule
}

func (self *Calendar) Tick() {
	// save states for visualization
	self.CurDay = self.curDay
	self.TimeOfDay = self.timeOfDay
	self.IsNight = (self.TimeOfDay+NightLen >= self.Today().Len)
	self.IsDusk = (self.TimeOfDay+NightLen == self.Today().Len)
	self.Table = self.days

	self.tick()
}

func (self *Calendar) CurSched() Matrix {
	if self.timeOfDay+NightLen >= self.Today().Len {
		return self.zeros
	}
	return self.Today().Sched
}

func (self *Calendar) String() string {
	buf := new(bytes.Buffer)
	fmt.Fprintf(buf, "%d {\n", len(self.days))

	for _, d := range self.days {
		buf.WriteString("    ")
		buf.WriteString(d.String())
		buf.WriteString("\n")
	}

	buf.WriteString("}")
	return buf.String()
}

type Schedule struct {
	// the demand estimated
	Demand Matrix

	// the window that the demand covers
	Window uint64

	Days []*Day

	// the suggested bandwidth throttling
	Bandw Matrix
}

func (self *Schedule) String() string {
	buf := new(bytes.Buffer)
	fmt.Fprintf(buf, "demand: x%d  %s\n", self.Window, self.Demand)
	for i, d := range self.Days {
		fmt.Fprintf(buf, "%d: %s\n", i, d.String())
	}
	fmt.Fprintf(buf, "bandw: %s\n", self.Bandw)
	return buf.String()
}

// Plans for the upcoming week if it is at the week boundary.
// Returns the newly generated schedule, or nil when it is not a week
// start.
func (self *Calendar) Plan(s Scheduler, e Estimator, r Matrix) *Schedule {
	if self.needSchedule() {
		demand, win := e.Estimate()
		assert(demand != nil)
		days, bandw := s.Schedule(r, demand, win)

		if days == nil || len(days) == 0 || bandw == nil {
			panic("bug")
		}

		for i, day := range days {
			if e := day.Check(); e != nil {
				log.Fatalf("day %d in schedule, %v", i, e)
			}
			day.ApplyBandw(bandw)
		}

		self.days = days

		return &Schedule{
			Demand: demand,
			Window: win,
			Days:   self.days,
			Bandw:  bandw,
		}
	}

	return nil
}
