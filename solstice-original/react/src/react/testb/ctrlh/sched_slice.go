package ctrlh

import (
	"log"
	"react/conf"
	"react/testb/setup"
)

func pick(picked []bool, hosts []int) int {
	for _, h := range hosts {
		if !picked[h] {
			picked[h] = true
			return h
		}
	}

	panic("nothing to pick")
}

func newDests() []int {
	ret := make([]int, setup.Nhost)
	for i := range ret {
		ret[i] = -1
	}
	return ret
}

func mapDests(d *SchedDay) []int {
	if !d.Check() {
		panic("invalid mapping")
	}

	ret := newDests()

	for _, lane := range d.Lanes {
		ret[lane.Src] = lane.Dest
	}

	return ret
}

func fillDestsHosts(dests []int, hosts []int) {
	if hosts == nil {
		hosts = conf.Conf.Hosts
	}

	srcPicked := make([]bool, setup.Nhost)
	destPicked := make([]bool, setup.Nhost)

	nmap := 0
	for src, dest := range dests {
		if dest >= 0 {
			srcPicked[src] = true
			destPicked[dest] = true
			nmap++
		}
	}

	nsrc, ndest := 0, 0
	for _, h := range hosts {
		if srcPicked[h] {
			nsrc++
		}
		if destPicked[h] {
			ndest++
		}
	}

	if nsrc != nmap || ndest != nmap {
		log.Fatalln("invalid dests for hosts", dests, hosts)
	}

	for _, h := range hosts {
		if !srcPicked[h] {
			dests[h] = pick(destPicked, hosts)
			srcPicked[h] = true
		}
	}
}

var fullTestbed = setup.FullTestbed()

func fillDests(dests []int) {
	fillDestsHosts(dests, fullTestbed)
}

func absSet(src, dest int, bits []byte) {
	if dest < 0 {
		panic("dest < 0")
	}
	bits[src] |= (0x1 << uint(dest))
}

func relDest(src, dest int) int {
	if dest < 0 {
		panic("dest < 0")
	}
	return (dest + setup.Nhost - src) % setup.Nhost
}

func relSet(src, dest int, bits []byte) {
	if src == dest {
		return
	}
	if !setup.CheckId(src) || !setup.CheckId(dest) {
		panic("invalid id")
	}

	absSet(src, relDest(src, dest), bits)
}

func relSets(dests []int, bits []byte) {
	for src, dest := range dests {
		if dest < 0 {
			continue
		}
		relSet(src, dest, bits)
	}
}

func absSets(dests []int, bits []byte) {
	for src, dest := range dests {
		if dest < 0 {
			continue
		}
		absSet(src, dest, bits)
	}
}

func setOnes(bits []byte) {
	for i := range bits {
		bits[i] |= 0x1
	}
}

func setAll(bits []byte) {
	for i := range bits {
		bits[i] = 0xff
	}
}

type schedSlice struct {
	t       uint64
	daywl   []uint8
	nightwl []uint8
	circwl  []uint8
	packbl  []uint8
	portMap []uint8
}

func newSchedSlice(t uint64) *schedSlice {
	ret := new(schedSlice)
	ret.t = t
	ret.daywl = make([]uint8, setup.Nhost)
	ret.nightwl = make([]uint8, setup.Nhost)
	ret.circwl = make([]uint8, setup.Nhost)
	ret.packbl = make([]uint8, setup.Nhost)
	ret.portMap = make([]uint8, setup.Nport)

	return ret
}

func TickToCycle(tick uint64) uint64 {
	return tick * setup.NanosPerTick * 5 / 32
}

func (self *schedSlice) writeTo(p *Packet) {
	p.Uint64(TickToCycle(self.t))
	p.Bytes(self.daywl)
	p.Bytes(self.nightwl)
	p.Bytes(self.circwl)
	p.Bytes(self.packbl)
	p.Bytes(self.portMap)
}
