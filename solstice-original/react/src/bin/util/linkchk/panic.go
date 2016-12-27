package main

import (
	"errors"
)

var errAssert = errors.New("assertion failed")
var errFail = errors.New("program failed")

func noErr(err error) {
	if err != nil {
		panic(err)
	}
}

func assert(cond bool) {
	if !cond {
		panic(errAssert)
	}
}

func fail() {
	panic(errFail)
}
