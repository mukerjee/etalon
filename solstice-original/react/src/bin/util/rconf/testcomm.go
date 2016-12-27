package main

import (
	"fmt"
)

type TestComm struct{}

func (s TestComm) Send(p []byte) {
	hprint(p)
	fmt.Println()
}
