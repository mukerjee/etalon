package main

import (
	"fmt"
	"react/util/pacer"
)

func main() {
	listener := pacer.NewListener()

	for {
		msg := listener.Recv()
		fmt.Println(string(msg))
	}
}
