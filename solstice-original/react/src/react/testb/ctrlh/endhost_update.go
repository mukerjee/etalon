package ctrlh

import (
	"react/testb/setup"
)

type endHostUpdate struct {
	weekId     uint32
	rateValid  uint8
	remapValid uint8
	doRemap    uint8
	applyNow   uint8
	rates      []uint8
	remaps     []uint8
}

func newEndhostUpdate(week uint32) *endHostUpdate {
	ret := new(endHostUpdate)
	ret.weekId = week
	ret.rates = make([]uint8, setup.Nhost)
	ret.remaps = make([]uint8, setup.Nhost)

	return ret
}

func (self *endHostUpdate) writeTo(p *Packet) {
	p.Uint32BE(setup.UpdateMagic)
	p.Uint32(self.weekId)
	p.Byte(self.rateValid)
	p.Byte(self.remapValid)
	p.Byte(self.doRemap)
	p.Byte(self.applyNow)
	p.Bytes(self.rates)
	p.Bytes(self.remaps)
}

func (self *endHostUpdate) Len() int {
	return 12 + setup.Nhost*2
}
