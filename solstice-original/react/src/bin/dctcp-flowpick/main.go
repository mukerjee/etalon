package main

import (
	"encoding/json"
	"fmt"
	"io"
	"os"

	"react/sim/flows"
)

func ne(e error) {
	if e != nil {
		panic(e)
	}
}

func as(cond bool) {
	if !cond {
		panic("bug")
	}
}

func main() {
	fin, e := os.Open("flows.log")
	ne(e)
	defer fin.Close()

	var entry flows.FlowRecord

	dec := json.NewDecoder(fin)

	bgOut, e := os.Create("bgflows")
	ne(e)
	defer bgOut.Close()

	qOut, e := os.Create("queries")
	ne(e)
	defer qOut.Close()

	fmt.Fprintf(bgOut, "size start dura\n")
	fmt.Fprintf(qOut, "size start dura\n")

	count := make(map[int]int)

	for {
		e = dec.Decode(&entry)
		if e == io.EOF {
			break
		}
		ne(e)

		if entry.Size < 5e5 {
			as(entry.End >= entry.Start)
			duration := entry.End - entry.Start
			if duration > 1e3 {
				fmt.Println(entry.Id)
			}
		}
		count[entry.From*32+entry.To]++
	}

	for i := 0; i < 32; i++ {
		for j := 0; j < 32; j++ {
			fmt.Println(i, j, count[i*32+j])
		}
	}
}
