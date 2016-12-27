package main

import (
	"bytes"
	"fmt"
	"strconv"
	"strings"
)

const macAddrSize = 6

type MacAddr []byte

var (
	broadcastMac = ParseMac("ff:ff:ff:ff:ff:ff")
	emptyMac     = ParseMac("00:00:00:00:00:00")
)

func isMacSep(r rune) bool {
	if r == ':' {
		return true
	}
	if r == '-' {
		return true
	}
	return false
}

func ParseMac(s string) MacAddr {
	ret := make([]byte, macAddrSize)

	fields := strings.FieldsFunc(s, isMacSep)
	assert(len(fields) == macAddrSize)

	for i := range ret {
		b, err := strconv.ParseUint(fields[i], 16, 8)
		noErr(err)
		ret[i] = byte(b)
	}

	return ret
}

func (self MacAddr) String() string {
	buf := new(bytes.Buffer)
	for i, b := range self {
		if i > 0 {
			fmt.Fprintf(buf, ":")
		}
		fmt.Fprintf(buf, "%02x", b)
	}

	return buf.String()
}
