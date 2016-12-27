package bvn

import (
	_log "log"
)

var (
	LogEnable = false
)

func logln(v ...interface{}) {
	if !LogEnable {
		return
	}
	_log.Println(v...)
}

func logf(fmt string, v ...interface{}) {
	if !LogEnable {
		return
	}
	_log.Printf(fmt, v...)
}
