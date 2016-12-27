package react

import "fmt"

type TimeView struct {
}

var _ View = new(TimeView)

func NewTimeView() *TimeView {
	ret := new(TimeView)
	return ret
}

func (self *TimeView) Reset(n int) {
	fmt.Println()
}

func (self *TimeView) Time(t uint64) {
	fmt.Print(CSI, "A", CSI, 0, "G")
	fmt.Print("t=", t, CSI, "K")
	fmt.Println()
}

func (self *TimeView) ConfigTime(_, _, _ uint64)         {}
func (self *TimeView) ConfigBandw(_, _ uint64)           {}
func (self *TimeView) MatrixClear(_ int)                 {}
func (self *TimeView) MatrixTitle(_ int, _ string)       {}
func (self *TimeView) MatrixNorm(_ int, _ uint64)        {}
func (self *TimeView) Matrix(_ int, _ Matrix)            {}
func (self *TimeView) SchedClearAfter(_ int)             {}
func (self *TimeView) Sched(_ int, _ *Day)               {}
func (self *TimeView) SchedTime(_ int, _ uint64, _ bool) {}
func (self *TimeView) FlowClearAfter(_ int)              {}
func (self *TimeView) Flow(_ int, _ *Flow)               {}
func (self *TimeView) Message(s string)                  {}
func (self *TimeView) Close()                            {}
