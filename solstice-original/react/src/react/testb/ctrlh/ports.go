package ctrlh

import (
	"react/testb/setup"
)

type Ports struct {
	Input  []uint8
	Output []uint8
}

func MordiaPorts() *Ports {
	return &Ports{
		[]uint8{55, 56, 57, 58, 39, 40, 41, 42}, // In
		[]uint8{7, 6, 5, 4, 3, 2, 1, 0},         // Out
	}
}

const (
	SouthEast = iota
	SouthWest
	NorthEast
	NorthWest
)

const (
	PortD0 = iota
	PortD1
	PortD2
	PortD3

	PortE0
	PortE1
	PortE2
	PortE3

	PortF0
	PortF1
	PortF2
	PortF3
)

var mindspdPorts = []*Ports{
	// SouthEast
	{
		[]uint8{23, 21, 22, 20, 12, 14, 13, 15, 29, 28, 31, 30},
		[]uint8{47, 45, 46, 44, 36, 38, 37, 39, 5, 4, 7, 6},
	},

	// SouthWest
	{
		[]uint8{47, 45, 46, 44, 36, 38, 37, 39, 5, 4, 7, 6},
		[]uint8{23, 21, 22, 20, 12, 14, 13, 15, 29, 28, 31, 30},
	},

	// NorthEast
	{
		[]uint8{35, 33, 34, 32, 40, 42, 41, 43, 17, 16, 19, 18},
		[]uint8{11, 9, 10, 8, 0, 2, 1, 3, 25, 24, 27, 26},
	},

	// NorthWest
	{
		[]uint8{35, 33, 34, 32, 40, 42, 41, 43, 17, 16, 19, 18},
		[]uint8{11, 9, 10, 8, 0, 2, 1, 3, 25, 24, 27, 26},
	},
}

type MindspdConn struct {
	Master int
	Slave  int
	Txs    []uint8
	Rxs    []uint8
}

func (self *MindspdConn) mindspdPorts() *Ports {
	ret := &Ports{
		Input:  make([]uint8, 8),
		Output: make([]uint8, 8),
	}

	for i := 0; i < 8; i++ {
		pos := 0
		if i < 4 {
			pos = self.Master
		} else {
			pos = self.Slave
		}

		ret.Input[i] = mindspdPorts[pos].Input[self.Txs[i]]
		ret.Output[i] = mindspdPorts[pos].Output[self.Rxs[i]]
	}

	return ret
}

const (
	nAirmax = 4
	nSFP    = 12
)

func (self *MindspdConn) MindspdPorts() *Ports {
	if setup.Nhost != 8 {
		panic("nhost must be 8 for mindspeed")
	}
	if self.Master == self.Slave {
		panic("master is same to slave")
	}
	if self.Master < 0 || self.Master >= nAirmax {
		panic("invalid master")
	}
	if self.Slave < 0 || self.Slave >= nAirmax {
		panic("invalid slave")
	}
	if len(self.Txs) != setup.Nhost || len(self.Rxs) != setup.Nhost {
		panic("invalid lane count")
	}

	for _, tx := range self.Txs {
		if tx < 0 || tx >= nSFP {
			panic("invalid tx lane id")
		}
	}

	for _, rx := range self.Rxs {
		if rx < 0 || rx >= nSFP {
			panic("invalid rx lane id")
		}
	}

	return self.mindspdPorts()
}
