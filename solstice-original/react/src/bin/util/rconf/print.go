package main

import (
	"fmt"
)

func hprint(buf []byte) {
	for i, c := range buf {
		fmt.Printf("%02x", c)
		if (i+1)%16 == 0 {
			fmt.Printf("\n")
		} else if (i+1)%8 == 0 {
			fmt.Printf("  ")
		} else if (i+1)%4 == 0 {
			fmt.Printf(" ")
		}
	}
	if len(buf)%16 != 0 {
		fmt.Println()
	}
}

type Printer struct{}

func (s Printer) Send(p []byte) {
	hprint(p)
}
