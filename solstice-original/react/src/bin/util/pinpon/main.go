package main

import (
	"flag"
	"fmt"
	"net"
)

var (
	peerHost   = "localhost"
	startFirst = false
)

func parseFlags() {
	flag.StringVar(&peerHost, "peer", peerHost, "the other peer")
	flag.BoolVar(&startFirst, "start", startFirst, "send first")

	flag.Parse()
}

func noErr(e error) {
	if e != nil {
		panic(e)
	}
}

func main() {
	parseFlags()

	laddr, err := net.ResolveUDPAddr("udp4", ":3370")
	noErr(err)

	raddr, err := net.ResolveUDPAddr("udp4", peerHost+":3370")
	noErr(err)

	p := make([]byte, 100)

	conn, err := net.ListenUDP("udp4", laddr)
	noErr(err)

	if startFirst {
		_, err := conn.WriteTo(p, raddr)
		noErr(err)
	}

	i := 0
	for {
		_, recvAddr, err := conn.ReadFrom(p)
		noErr(err)
		fmt.Println("recved", i)
		i++

		_, err = conn.WriteTo(p, recvAddr)
		noErr(err)
	}
}
