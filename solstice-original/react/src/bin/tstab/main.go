package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strings"
)

func noError(e error) {
	if e != nil {
		log.Fatal(e)
	}
}

func assert(cond bool) {
	if !cond {
		log.Fatal("assert")
	}
}

func main() {
	pin := "tsched.dat"
	pout := "tsched.tab"
	fin, e := os.Open(pin)
	noError(e)
	defer func() { noError(fin.Close()) }()

	fout, e := os.Create(pout)
	noError(e)
	defer func() { noError(fout.Close()) }()

	scanner := bufio.NewScanner(fin)
	i := 0
	if !scanner.Scan() {
		panic("header missing")
	}
	header := scanner.Text()
	noError(scanner.Err())

	fmt.Fprint(fout, "dem cap seed")
	for _, s := range strings.Fields(header) {
		for _, f := range strings.Fields(`
			nday circ pend pack time
		`) {
			fmt.Fprintf(fout, " %s.%s", s, f)
		}
	}
	fmt.Fprintln(fout)

	for scanner.Scan() {
		line := scanner.Text()
		line = strings.TrimSpace(line)
		if len(line) == 0 {
			fmt.Fprintln(fout)
			i = 0
			continue
		}

		fields := strings.Fields(line)
		assert(len(fields) > 1)
		for _, f := range fields[1:] {
			if i > 0 {
				fmt.Fprint(fout, " ")
			}
			fmt.Fprint(fout, f)
			i++
		}
	}

	noError(scanner.Err())
}
