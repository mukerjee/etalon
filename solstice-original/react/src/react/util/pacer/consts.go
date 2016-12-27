package pacer

import (
	"fmt"
	"net"
)

const BroadcastNet = "172.22.16.255"
const Port = 7337
const DefaultNic = "eth0"

var broadcastAddr = func() *net.UDPAddr {
	s := fmt.Sprintf("%s:%d", BroadcastNet, Port)
	addr, err := net.ResolveUDPAddr("udp4", s)
	noErr(err)

	return addr
}()

var listenInterface = func() *net.Interface {
	ifs, err := net.Interfaces()
	noErr(err)
	for _, i := range ifs {
		if i.Name == DefaultNic {
			return &i
		}
	}
	return nil
}()
