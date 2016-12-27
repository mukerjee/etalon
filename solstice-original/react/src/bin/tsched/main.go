package main

import (
	"flag"
	"io"
	"io/ioutil"
	"log"
	"os"
	"runtime"

	. "react/sim/config"
)

const defaultConfig = `
	{ "Pattern": "flows", "Norm": 3000 }
	{ "Noise": 200, "Max": 2700, "Nflow": 24 }
	{ "Aligns": [ true ], "Stuffers": ["quick"], "Slicers": ["any"] }
`

func setup() {
	SetNhost(8)
	LinkBw = 1000
	PackBw = 100
	WeekLen = 3000
	NightLen = 0
	MinDayLen = 100
	AvgDayLen = 300
	TickPerGrid = 1
}

func loadConfig() string {
	bytes, e := ioutil.ReadFile("tsched.conf")
	if os.IsNotExist(e) {
		return defaultConfig
	}
	if e != nil {
		log.Fatal(e)
	}
	return string(bytes)
}

func main() {
	runtime.GOMAXPROCS(4)
	log.SetFlags(0)
	setup()
	path := flag.String("out", "", "output file path")
	flag.Parse()

	run, e := ParseRun(loadConfig())
	if e != nil {
		log.Fatal(e)
	}

	var out io.Writer
	if *path == "" {
		out = os.Stdout
	} else {
		fout, e := os.Create(*path)
		if e != nil {
			log.Fatal(e)
		}
		defer func() {
			e := fout.Close()
			if e != nil {
				log.Fatal(e)
			}
		}()
		out = fout
	}

	run.Run(out)
}
