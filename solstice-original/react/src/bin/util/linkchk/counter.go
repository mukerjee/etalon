package main

type Counter struct {
	source string

	count     uint64
	group     uint64
	perGroup  uint64
	totalLost uint64
}

func NewCounter(source string) *Counter {
	ret := new(Counter)
	ret.source = source
	return ret
}

func (self *Counter) Parse(p []byte) (stat *Stat) {
	thisSig := encoder.Uint64(p[0:8])
	if thisSig != SIG {
		return // ignore
	}

	if len(p) < 16 {
		return
	}

	thisGroup := encoder.Uint64(p[8:16])
	thisId := encoder.Uint32(p[16:24])
	thisPerGroup := encoder.Uint64(p[24:32])

	if thisGroup != self.group {
		if self.perGroup > 0 {
			if !(thisGroup == 0 && thisId == 0) {
				self.totalLost += self.perGroup - self.count
				stat = NewStat(self.source, self.group,
					self.count, self.perGroup, self.totalLost)
			}
		}

		self.count = 0
		self.perGroup = thisPerGroup
		self.group = thisGroup
	}

	if self.perGroup != thisPerGroup {
		return // ignore when per group is inconsistent
	}

	self.count++

	return
}
