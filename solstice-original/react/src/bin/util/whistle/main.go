package main

import (
	"fmt"
	"time"

	"react/util/pacer"
)

func main() {
	sender := pacer.NewSender()
	i := 0
	for {
		i++
		msg := fmt.Sprintf("step %d", i)
		sender.Broadcast([]byte(msg))
		fmt.Println(msg)
		time.Sleep(time.Second)
	}
}
