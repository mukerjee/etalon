package react

import "time"

type FrameControl struct {
	frameInterval time.Duration
	next          <-chan time.Time
}

func NewFrameControl(interval time.Duration) *FrameControl {
	ret := new(FrameControl)
	ret.frameInterval = interval
	return ret
}

func (self *FrameControl) Please() bool {
	if self.next == nil {
		self.next = time.After(self.frameInterval)
		return true
	}

	if len(self.next) == 0 {
		return false
	}

	<-self.next
	self.next = time.After(self.frameInterval)
	return true
}
