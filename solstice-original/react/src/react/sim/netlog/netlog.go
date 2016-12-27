package netlog

import (
	"log"
	"net"
)

var logger *log.Logger

func init() {
	conn, err := net.Dial("tcp", "localhost:10240")
	if err != nil {
		return
	}
	f := log.Ldate | log.Ltime | log.Lshortfile
	logger = log.New(conn, "", f)
}

func Fatal(v ...interface{}) {
	if logger == nil {
		return
	}
	logger.Fatal(v...)
}

func Print(v ...interface{}) {
	if logger == nil {
		return
	}
	logger.Print(v...)
}

func Printf(f string, v ...interface{}) {
	if logger == nil {
		return
	}
	logger.Printf(f, v...)
}

func Println(v ...interface{}) {
	if logger == nil {
		return
	}
	logger.Println(v...)
}
