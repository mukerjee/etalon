package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	raw "rawpack"
	"react/testb/setup"
)

func noError(e error) {
	if e != nil {
		panic(e)
	}
}

var enc = binary.BigEndian

func vlog(prefix string, buf []byte) {
	n := len(buf)
	fmt.Printf("// %d bytes\n", n)
	if n%8 != 0 {
		panic("error: vlog() buffer must align with 8 bytes")
	}

	for i := 0; i < n; i += 8 {
		fmt.Printf("localparam [63:0] %s_hdr%d = 64'h", prefix, i/8)
		for j := 0; j < 8; j++ {
			if j > 0 && j%2 == 0 {
				fmt.Print("_")
			}
			fmt.Printf("%02x", buf[i+7-j])
		}
		fmt.Println(";")
	}
}

func buildNewWeek() {
	buf := new(bytes.Buffer)
	binary.Write(buf, enc, uint32(setup.WeekendMagic))
	binary.Write(buf, enc, uint16(0))
	binary.Write(buf, enc, uint64(0))
	binary.Write(buf, enc, uint64(0))

	pack := &raw.UDPPacket{
		DestMAC:  raw.BroadcastMAC,
		SrcMAC:   raw.MAC(setup.CtrlMAC),
		SrcIP:    raw.IP(setup.CtrlIP),
		DestIP:   raw.IP(setup.BroadcastIP),
		SrcPort:  setup.CtrlPort,
		DestPort: setup.CtrlPort,
	}

	p := pack.Make(buf.Bytes())
	vlog("newweek", p)
}

func buildStart() {
	buf := new(bytes.Buffer)
	binary.Write(buf, enc, uint32(setup.LoadgStart))
	binary.Write(buf, enc, uint16(0))
	binary.Write(buf, enc, uint64(0))
	binary.Write(buf, enc, uint64(0))

	pack := &raw.UDPPacket{
		DestMAC:  raw.BroadcastMAC,
		SrcMAC:   raw.MAC(setup.CtrlMAC),
		SrcIP:    raw.IP(setup.CtrlIP),
		DestIP:   raw.IP(setup.BroadcastIP),
		SrcPort:  setup.LoadgPort,
		DestPort: setup.LoadgPort,
	}

	p := pack.Make(buf.Bytes())
	vlog("start", p)
}

func main() {
	fmt.Println("// new week")
	buildNewWeek()
	fmt.Println()

	fmt.Println("// loadg start")
	buildStart()
}
