package react

import "math/rand"

type DumbHosts struct {
	means     []int
	rand      *rand.Rand
	UseRandom bool
}

func NewDumbHosts(seed int64, means []int) *DumbHosts {
	ret := new(DumbHosts)
	ret.means = means
	ret.rand = rand.New(rand.NewSource(seed))
	return ret
}

var _ Hosts = new(DumbHosts)

func (self *DumbHosts) Push(packet *Packet) {
	// this dumb packet sender is stateless
	// so it does nothing for the downlink do nothing
}

func (self *DumbHosts) Tick(sender Sender) {
	var size, lane int

	for i := 0; i < nhost; i++ {
		for j := 0; j < nhost; j++ {
			k := (j + nhost - i) % nhost
			lane = i*nhost + j
			size = self.means[k]

			if self.UseRandom {
				if size > 20 {
					noise := poisson(self.rand, 20) - 20
					size += noise
				} else {
					size = poisson(self.rand, size)
				}
			}
			sender.Send(NewPacket(uint64(size), lane))
		}
	}
}
