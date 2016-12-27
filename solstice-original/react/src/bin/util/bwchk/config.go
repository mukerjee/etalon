package main

import (
	"flag"
	"react/util/hosts"
	"time"
)

var port = 8800
var bufSize = "64m"
var hisLen = 600

var hostList hosts.Hosts
var hostsPath = ""
var connects = "all"
var sampleIntervalStr = "200ms"
var sampleInterval = time.Second / 5

func parseFlags() {
	flag.StringVar(&bufSize, "bufsize", bufSize, "buffer size")
	flag.IntVar(&port, "port", port, "port")
	flag.IntVar(&hisLen, "hislen", hisLen, "average window size")
	flag.StringVar(&hostsPath, "hosts", hostsPath, "file of the host list")
	flag.StringVar(&connects, "conn", connects, "connect pattern")
	flag.StringVar(&sampleIntervalStr, "sample", sampleIntervalStr,
		"sample interval string")

	flag.Parse()

	if len(hostsPath) == 0 {
		hostList = hosts.Default
	} else {
		hostList = hosts.OpenFile(hostsPath)
	}

	checkFlags()
}

func checkFlags() {
	assert(hostList != nil)
	assert(parseBufSize(bufSize) > 0)
	assert(hisLen > 0)
	assert(port > 0 && port < 65536)

	var err error
	sampleInterval, err = time.ParseDuration(sampleIntervalStr)
	noErr(err)
	assert(sampleInterval > 0)
	assert(sampleInterval <= time.Second*2)
}
