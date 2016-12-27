package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
)

func noError(e error) {
	if e != nil {
		panic(e)
	}
}

func assert(cond bool) {
	if !cond {
		panic("bug")
	}
}

var hostMacs = []uint8{
	0x8d, 0x41, 0x99, 0x0d,
	0x7d, 0xc1, 0x21, 0x95,
	0xff,
}

func hostId(ad uint8) int {
	for i, mac := range hostMacs {
		if mac == ad {
			return i
		}
	}
	return -1
}

type throughput []uint64

const nslot = 250
const tmax = 3e9
const tbase = 1.7e9 + 3e6

func main() {
	f, e := os.Open("../dmp")
	noError(e)
	defer func() { noError(f.Close()) }()
	buf := make([]uint64, 10)

	scanner := bufio.NewScanner(f)
	lastSerial := uint8(0)
	throughputs := make([]throughput, 7)
	for i := range throughputs {
		throughputs[i] = throughput(make([]uint64, nslot))
	}

	ftr, e := os.Create("trace")
	noError(e)
	defer func() { noError(ftr.Close()) }()

	for scanner.Scan() {
		line := scanner.Text()
		fields := strings.Fields(line)
		nfield := len(fields)
		// myrits := fields[0]
		_size := fields[nfield-1]
		data := fields[1 : nfield-1]

		ndata := len(data)
		assert(ndata > 0)
		assert(ndata < 10)
		idata := buf[:ndata]

		for i, d := range data {
			idata[i], e = strconv.ParseUint(d, 16, 64)
			noError(e)
		}
		size, e := strconv.Atoi(_size)
		noError(e)
		assert(size >= 60)

		serial := uint8((idata[0] >> 56) & 0xff)
		if lastSerial != 0 && serial != lastSerial+1 {
			panic(fmt.Errorf("serial jump: %d -> %d", lastSerial, serial))
		}
		lastSerial = serial
		dest := uint8((idata[0] >> 48) & 0xff)
		src := uint8((idata[1] >> 24) & 0xff)

		// proto := (idata[1] >> 32) & 0xffff
		destId := hostId(dest)
		srcId := hostId(src)

		ts := idata[0] & ((1 << 48) - 1)
		tsnano := float64(ts) * 6.4
		if tsnano > tmax {
			break
		}
		// assert(tsnano <= tmax)
		tslot := uint64(tsnano / (tmax / nslot))

		if destId == 7 && srcId < 7 && srcId >= 0 && size > 100 {
			throughputs[srcId][tslot] += uint64(size)
		}

		if tsnano > tbase && tsnano < tbase + 20e6 {
			typ := 0
			if destId == 8 && srcId == -1 && size == 64 {
				typ = 2 // week ends
			} else if size == 60 && srcId == 8 && destId == -1 {
				typ = 4 // PFC
			} else if srcId == 8 && destId == 8 {
				typ = 5 // week schedule updates
			} else if destId == 7 && size > 100 {
				typ = 1 // ACKs
			}

			if destId < 0 || srcId < 0 {
				fmt.Printf("%.3f %02x->%02x %d->%d sz=%d typ=%d\n",
					tsnano - tbase, src, dest, srcId, destId, size, typ)
				// fmt.Println(strings.TrimSpace(line))
			} else if destId != 7 {
				fmt.Printf("%.3f %02x->%02x %d->%d sz=%d typ=%d\n",
					tsnano - tbase, src, dest, srcId, destId, size, typ)
				// fmt.Println(strings.TrimSpace(line))
			}

			
			fmt.Fprintf(ftr, "%f %d %d %d\n",
				(tsnano - tbase) / 1e3, srcId, typ, size,
			)
			// fmt.Println(strings.TrimSpace(line))
		}
	}
	noError(scanner.Err())

	fout, e := os.Create("thput")
	noError(e)
	defer func() { noError(fout.Close()) }()

	norm := tmax * 1250 / 1000 / nslot / 10
	for i := 0; i < nslot; i++ {
		fmt.Fprintf(fout, "%d", i * tmax / nslot)
		sum := uint64(0)
		for j := 0; j < 7; j++ {
			th := throughputs[j][i]
			sum += th
			fmt.Fprintf(fout, " %f", float64(sum) / float64(norm))
		}
		fmt.Fprintln(fout)
	}
}
