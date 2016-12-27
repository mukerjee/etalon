package hosts

import (
	"net"
	"strings"
)

func IsIP(s string) bool {
	return net.ParseIP(s) != nil
}

func IsSelf(ip string) bool {
	assert(IsIP(ip))
	addrs, err := net.InterfaceAddrs()
	noErr(err)

	for _, addr := range addrs {
		if strings.HasPrefix(addr.String(), ip) {
			return true
		}
	}

	return false
}
