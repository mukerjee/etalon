package main

import (
	"encoding/binary"
	"net"
)

func noError(e error) {
	if e != nil {
		panic(e)
	}
}

func main() {
	conn, e := net.ListenUDP("udp4", nil)
	noError(e)
	defer func() { noError(conn.Close()) }()

	p := make([]byte, 4)
	binary.BigEndian.PutUint32(p[:4], uint32(0x372353ae))

	addr := &net.UDPAddr{
		IP:   net.ParseIP("192.168.1.255"),
		Port: 8203,
	}

	_, e = conn.WriteToUDP(p, addr)
	noError(e)
}
