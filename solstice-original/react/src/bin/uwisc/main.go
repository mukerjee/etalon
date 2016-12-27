package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"os"

	"react/sim/config"
	"react/sim/density"
	"react/sim/structs"
)

var enc = binary.LittleEndian

type Record struct {
	t     uint64
	src   uint16
	dest  uint16
	nbyte uint32
}

func ne(e error) {
	if e != nil {
		panic(e)
	}
}

func as(cond bool) {
	if !cond {
		panic("bug")
	}
}

func LoadRecords(f string) []*Record {
	var ret []*Record

	fin, e := os.Open(f)
	ne(e)
	buf := make([]byte, 16)

	for {
		_, e := io.ReadFull(fin, buf)
		if e == io.EOF {
			break
		}
		ne(e)

		r := &Record{
			t:     enc.Uint64(buf[0:8]),
			src:   enc.Uint16(buf[8:10]),
			dest:  enc.Uint16(buf[10:12]),
			nbyte: enc.Uint32(buf[12:16]),
		}

		ret = append(ret, r)
	}

	ne(fin.Close())

	return ret
}

const (
	TslotSize  = 3e6
	MaxingSize = 5e9
)

var (
	traceId = flag.Int("id", 1, "trace id")
)

func main() {
	flag.Parse()
	var fname string
	if *traceId == 1 {
		fname = "dat/univ1"
		config.SetNhost(1181)
	} else {
		as(*traceId == 2)
		fname = "dat/univ2"
		config.SetNhost(2067)
	}

	fmt.Fprintf(os.Stderr, "loading %s\n", fname)
	rs := LoadRecords(fname)
	fmt.Fprintf(os.Stderr, "file loaded\n")

	m := structs.NewSparse()
	d := new(density.Density)
	dmax := new(density.Density)

	curSlot := uint64(0)
	curMax := uint64(0)

	fmt.Printf("nze be neph bph nzph50 nzph90 bph50 bph90\n")
	for _, r := range rs {
		tslot := r.t / TslotSize
		tmax := r.t / MaxingSize
		if curSlot == 0 {
			curSlot = tslot
		}

		if curSlot != tslot {
			d.CountSparse(m)
			dmax.Max(d)
			m.Clear()

			curSlot = tslot
		}

		for tmax > curMax {
			if curMax > 0 {
				dmax.Max(d)
				fmt.Printf("%d %d %d %d %d %d %d %d\n",
					dmax.NonZeroElements, dmax.BigElements,
					dmax.NonZeroPerHostMax, dmax.BigPerHostMax,
					dmax.NonZeroPerHost50, dmax.NonZeroPerHost90,
					dmax.BigPerHost50, dmax.BigPerHost90,
				)
				dmax.Clear()
			}

			if curMax == 0 {
				curMax = tmax
			} else {
				fmt.Fprintf(os.Stderr, "%d\n", curMax)
				curMax++
			}
		}

		m.AddAt(int(r.src), int(r.dest), uint64(r.nbyte))
	}
}
