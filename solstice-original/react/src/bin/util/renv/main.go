package main

import (
	"fmt"
	"net"
	"os"
	"strings"
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

func main() {
	hostname, err := os.Hostname()
	if err != nil {
		panic(err)
	}
	if len(hostname) < len("reactorx") || !strings.HasPrefix(hostname, "reactor") {
		return
	}
	hostid := hostname[len("reactor")]
	if !(hostid >= '0' && hostid <= '7') {
		panic("not reactor[0-7].")
	}

	ifaces, err := net.Interfaces()
	if err != nil {
		panic(err)
	}

	ip := fmt.Sprintf("192.168.1.5%c", hostid)
	name := fmt.Sprintf("reactor%c", hostid)
	id := fmt.Sprintf("%c", hostid)

	var iface, mac string
	found := false
	for _, i := range ifaces {
		if len(i.Name) == 3 && i.Name[0] == 'i' && isOdd(i.Name[2]) {
			if found {
				panic("multiple ixx found.\n")
			}

			iface = i.Name
			mac = i.HardwareAddr.String()
			found = true
		}
	}

	if !found {
		panic("interface ixx not found.\n")
	}

	if len(os.Args) > 1 && os.Args[1] == "arp" {
		fmt.Printf("sudo arp -s %s %s -i $REACTOR_IFACE\n", ip, mac)
		return
	}

	fmt.Printf("export REACTOR_HOSTID=%s\n", id)
	fmt.Printf("export REACTOR_HOSTNAME=%s\n", name)
	fmt.Printf("export REACTOR_IP=%s\n", ip)
	fmt.Printf("export REACTOR_IFACE=%s\n", iface)
	fmt.Printf("export REACTOR_MAC=%s\n", mac)
}
