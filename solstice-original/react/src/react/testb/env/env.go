package env

import (
	"net"
	"os"
	"strings"

	"react/testb/setup"
)

var (
	Id    int
	Host  string
	Ip    string
	Iface string
	Mac   net.HardwareAddr

	LocalIp    string
	LocalIface string
)

func isOdd(r uint8) bool {
	switch r {
	case '1', '3', '5', '7', '9':
		return true
	case 'b', 'd', 'f':
		return true
	}

	return false
}

func addrCopy(a net.HardwareAddr) net.HardwareAddr {
	ret := net.HardwareAddr(make([]byte, len(a)))
	copy(ret, a)
	return ret
}

func init() {
	hostname, err := os.Hostname()
	if err != nil {
		panic(err)
	}
	if len(hostname) < len("reactorx") ||
		!strings.HasPrefix(hostname, "reactor") {
		return
	}

	hostid := hostname[len("reactor")]
	if !(hostid >= '0' && hostid <= '7') {
		panic("not reactor[0-7].")
	}

	id := int(hostid - '0')
	if !setup.CheckId(id) {
		panic("host id invalid")
	}

	ifaces, err := net.Interfaces()
	if err != nil {
		panic(err)
	}

	var iface string
	var mac net.HardwareAddr

	found := false
	for _, i := range ifaces {
		if len(i.Name) == 3 && i.Name[0] == 'i' && isOdd(i.Name[2]) {
			if found {
				panic("multiple ixx found.\n")
			}

			iface = i.Name
			mac = i.HardwareAddr
			found = true
		}
	}

	if !found {
		panic("interface ixx not found.\n")
	}

	Id = id
	Host = setup.Host(id)
	Ip = setup.Ip(id)

	Iface = iface
	Mac = addrCopy(mac)

	LocalIp = setup.LocalIp(id)
	LocalIface = setup.LocalIface(id)
}
