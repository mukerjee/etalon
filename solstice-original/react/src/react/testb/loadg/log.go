package loadg

import (
	"log"
)

var Verbose = false

func _logf(s string, a ...interface{}) {
	log.Printf(s, a...)
}

func logf(s string, a ...interface{}) {
	if !Verbose {
		return
	}
	_logf(s, a...)
}

func logln(s string) {
	logf("%s", s)
}

func logerr(e error) {
	if e != nil {
		_logf("error: %s", e.Error())
	}
}
