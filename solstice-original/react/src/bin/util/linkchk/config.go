package main

import (
	"encoding/binary"
	"flag"
	"react/util/hosts"
	"time"
)

const SIG = 0x3375a41779f3520c

var encoder = binary.LittleEndian

var port = 8800
var perGroup uint64 = 1000
var intervalStr = "1ms"
var interval = time.Millisecond
var packetLen = 1000

var hostList hosts.Hosts
var hostsPath = ""
var connects = "all"

const statTTL = 20

func checkFlags() {
	assert(hostList != nil)
	assert(0 < port && port < 65536)
	assert(perGroup > 1)
	assert(packetLen >= 32 && packetLen < 65536)

	var err error
	interval, err = time.ParseDuration(intervalStr)
	noErr(err)
	assert(interval > 0)
}

func parseFlags() {
	flag.IntVar(&port, "port", port, "port")
	flag.Uint64Var(&perGroup, "pergroup", perGroup, "packets per group")
	flag.IntVar(&packetLen, "plen", packetLen, "packet length")
	flag.StringVar(&intervalStr, "interval", intervalStr,
		"interval between packets")
	flag.StringVar(&hostsPath, "hosts", hostsPath, "host list file")
	flag.StringVar(&connects, "conn", connects, "hosts to connect")

	flag.Parse()

	if len(hostsPath) == 0 {
		hostList = hosts.Default
	} else {
		hostList = hosts.OpenFile(hostsPath)
	}

	checkFlags()
}
