package main

import (
	"fmt"
	"os"
	"os/signal"
	"runtime"
	"time"
)

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

		for i, host := range hostList {
			if host.IsSelf() {
				index := (i + 1) % len(hostList)
				clients[0] = NewClient(hostList[index].IP)
			}
		}
	}

	return
}

func main() {
	runtime.GOMAXPROCS(4)
	parseFlags()

	closed := make(chan os.Signal, 10)
	signal.Notify(closed, os.Interrupt)

	server := NewServer("")
	go server.serve(closed, hostList)

	clients := makeClients(connects)
	assert(clients != nil)

	ticker := time.NewTicker(interval)
	for len(closed) == 0 {
		<-ticker.C

		for _, c := range clients {
			if c == nil {
				continue
			}
			c.Tick()
		}
	}
	ticker.Stop()

	fmt.Println()
	time.Sleep(time.Second / 10)
}
