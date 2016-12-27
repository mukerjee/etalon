package main

import (
	"errors"
)

var errAssert = errors.New("assertion failed")

func assert(cond bool) {
	if !cond {
		panic(errAssert)
	}
}

func noErr(err error) {
	if err != nil {
		panic(err)
	}
}
