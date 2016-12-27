package main

import (
	"strconv"
)

func parseBufSize(s string) (ret int) {
	n := len(s)
	assert(n > 0)

	last := s[n-1]
	other := s[:n-1]

	var err error
	switch last {
	case 'g', 'G':
		ret, err = strconv.Atoi(other)
		ret *= 1024 * 1024 * 1024
	case 'm', 'M':
		ret, err = strconv.Atoi(other)
		ret *= 1024 * 1024
	case 'k', 'K':
		ret, err = strconv.Atoi(other)
		ret *= 1024
	default:
		ret, err = strconv.Atoi(s)
	}

	assert(err == nil)

	return
}
