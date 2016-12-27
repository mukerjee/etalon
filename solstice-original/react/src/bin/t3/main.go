package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"react/conf"
	"react/demand"
	"react/testb/setup"
)

func ne(e error) {
	if e != nil {
		panic(e)
	}
}

var (
	pathDem = flag.String("dem", "demand", "demand input")
)

func mprint(m []demand.Vector) {
	for _, v := range m {
		e := v.PrintTo(os.Stdout)
		ne(e)
	}
	fmt.Println()
}

func main() {
	conf.Load()
	flag.Parse()
	log.SetFlags(log.Ltime)

	dem, e := demand.Load(*pathDem)
	ne(e)

	c := dem.WindChan(setup.WeekLen)
	i := 0

	for m := range c {
		i++
		fmt.Printf("W%d\n", i)
		mprint(m)
	}

}
