// Package demand provides handy functions for creating Demand inputs
package demand

import (
	"encoding/binary"
	"io"

	dem "react/demand"
	"react/testb/setup"
)

// Returns a demand that matches the number of hosts of the testbed
func New() *dem.Demand {
	return dem.NewDemand(setup.Nhost)
}

var enc = binary.LittleEndian

func marshal(p *dem.Period, id int, buf []byte) {
	enc.PutUint64(buf[0:8], p.N)
	for i, r := range p.D[id] {
		index := 8 + i*2
		if r > 1250 {
			r = 1250
		}
		enc.PutUint16(buf[index:index+2], uint16(r))
	}
}

func WriteHost(w io.Writer, d *dem.Demand, id int) error {
	buf := make([]byte, 24)
	n := d.Nperiod()

	for i := 0; i < n; i++ {
		marshal(d.Period(i), id, buf)
		_, e := w.Write(buf)
		if e != nil {
			return e
		}
	}

	return nil
}
