package signal

import "os/signal"
import "os"

type Trigger chan int

func (self Trigger) sigTrig(in <-chan os.Signal) {
	for _ = range in {
		self.Set()
	}
}

func NewTrigger() Trigger {
	ret := make(chan int, 3)
	return ret
}

func NewSignal(sig os.Signal) Trigger {
	ret := NewTrigger()

	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, sig)
	go ret.sigTrig(sigs)

	return ret
}

func NewCtrlC() Trigger {
	return NewSignal(os.Interrupt)
}

func (self Trigger) Set() {
	if self.Get() {
		return
	}
	go func() { self <- 1 }()
}

func (self Trigger) Get() bool { return len(self) > 0 }
