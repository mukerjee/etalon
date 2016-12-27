package rawpack

import (
	"net"
)

func IP(s string) net.IP {
	ret := net.ParseIP(s)
	if ret == nil {
		panic("invalid IP")
	}
	ret = ret.To4()
	if ret == nil || len(ret) != 4 {
		panic("not ipv4")
	}
	return ret
}

func MAC(s string) net.HardwareAddr {
	ret, e := net.ParseMAC(s)
	if e != nil {
		panic(e)
	}

	return ret
}
