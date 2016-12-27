package main

type Flags struct {
	SendPFC        bool
	SendWeekend    bool
	SendWeekSignal bool
	SyncInternal   bool
}

func (self *Flags) Byte() uint8 {
	ret := uint8(0)
	if self.SendPFC {
		ret |= 0x1
	}
	if self.SendWeekend {
		ret |= 0x2
	}
	if self.SendWeekSignal {
		ret |= 0x4
	}
	if self.SyncInternal {
		ret |= 0x8
	}

	return ret
}
