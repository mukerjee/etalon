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
}

func hostId(ad uint8) int {
	if ad == 0xff {
		return 8
	}

	for i, mac := range hostMacs {
		if mac == ad {
			ret := (i + 4) % 8
			if ret == 7 {
				ret = 6
			}
			return ret
		}
	}
	return -1
}

type throughput []uint64

const nslot = 250
const tmax = 1e9
const tbase = 8e6 + 1.5e6 + .97e6
const tdur = 6.05e6

func main() {
	f, e := os.Open("../dmp")
	noError(e)
	defer f.Close()
	buf := make([]uint64, 10)

	scanner := bufio.NewScanner(f)
	lastSerial := uint8(0)

	ftr, e := os.Create("trace")
	noError(e)
	defer ftr.Close()

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
		if destId < 0 || srcId < 0 {
			fmt.Println(src, dest, srcId, destId, size)
			// fmt.Println(strings.TrimSpace(line))
		} else if destId != 7 {
			fmt.Println(src, dest, srcId, destId, size)
			// fmt.Println(strings.TrimSpace(line))
		}

		ts := idata[0] & ((1 << 48) - 1)
		tsnano := float64(ts) * 6.4
		if tsnano > tmax {
			break
		}
		if tsnano > tbase && tsnano < tbase + tdur {
			typ := 0
			if destId == 8 && srcId == -1 && size == 64 {
				typ = 2 // week ends
			} else if size == 60 && srcId == 8 && destId == -1 {
				typ = 4 // PFC
			} else if srcId == 8 && destId == 8 {
				typ = 5 // week schedule updates
			} else if srcId == -1 && destId == 8 {
				typ = 5 // like a hack
			}
			
			fmt.Fprintf(ftr, "%f %d %d %d\n",
				(tsnano - tbase) / 1e3, srcId, typ, size,
			)
			// fmt.Println(strings.TrimSpace(line))
		}
	}
	noError(scanner.Err())
}
