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

	for {
		e = dec.Decode(&entry)
		if e == io.EOF {
			break
		}
		ne(e)

		switch entry.Type {
		case "bg":
			fmt.Fprintf(bgOut, "%d %d %d\n",
				entry.Size,
				entry.Start,
				entry.End-entry.Start+1,
			)
		case "query":
			fmt.Fprintf(qOut, "%d %d %d\n",
				entry.Size,
				entry.Start,
				entry.End-entry.Start+1,
			)
		default:
			panic("bug")
		}
	}
}
