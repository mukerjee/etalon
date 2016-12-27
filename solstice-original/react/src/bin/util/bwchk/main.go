package main

import (
	"fmt"
	"react/util/hosts"
	"runtime"
	"time"
)

func selfIndex(list hosts.Hosts) (int, bool) {
	for i, host := range list {
		if host.IsSelf() {
			return i, true
		}
	}

	return 0, false
}

func getIP(i int) string {
	return hostList[i%len(hostList)].IP
}

func clientCount(clients []*Client) int {
	ret := 0
	for _, c := range clients {
		if c != nil {
			ret++
		}
	}
	return ret
}

func roundStart(clients []*Client, startWith int) {
	ticks := time.Tick(time.Second * 3 / 2)
	assert(clientCount(clients) > 0)
	assert(startWith >= 0 && startWith < len(clients))

	var last *Client

	for {
		for i := range clients {
			index := (i + startWith) % len(clients)
			c := clients[index]
			if c == nil {
				continue
			}

			if last != nil {
				last.Stop()
			}
			c.Start()
			last = c
			<-ticks // wait for a signal for next round
		}
	}
}

func makeClients(conn string) (clients []*Client) {
	switch conn {
	case "all":
		clients = make([]*Client, len(hostList))
		for i, host := range hostList {
			if host.IsSelf() {
				continue
			}
			clients[i] = NewClient(host.IP)
		}

	case "next":
		clients = make([]*Client, 1)
		i, found := selfIndex(hostList)
		if found {
			clients[0] = NewClient(getIP(i + 1))
		}

	case "mix":
		assert(len(hostList) >= 4)

		clients = make([]*Client, 3)
		i, found := selfIndex(hostList)
		if found {
			clients[0] = NewClient(getIP(i + 1))
			clients[1] = NewRepeatClient(getIP(i+2), 1024, time.Millisecond)
			clients[2] = NewRepeatClient(getIP(i+3), 1024, time.Millisecond)
		}

	case "pace":
		clients = make([]*Client, len(hostList))
		startWith := 0
		for i, host := range hostList {
			if host.IsSelf() {
				startWith = i
				continue
			}
			clients[i] = NewClient(host.IP)
			clients[i].Stop() // all stop at first
		}

		go roundStart(clients, startWith)
	}

	return
}

func main() {
	runtime.GOMAXPROCS(8)
	parseFlags()

	server := NewServer("", hostList)
	go server.Serve()

	clients := makeClients(connects)
	for _, c := range clients {
		if c == nil {
			continue
		}
		go c.Run()
	}

	reporter := NewReporter(len(hostList))
	reporter.Serve()

	time.Sleep(time.Second / 10)
	fmt.Println()
}
