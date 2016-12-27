// The loadgen agent that runs on endhosts
package main

import (
	"fmt"
	"os"

	"react/testb/loadg"
	"react/testb/setup"
)

func epanic(e error) {
	if e != nil {
		fmt.Fprintf(os.Stderr, "error: %s\n", e)
		os.Exit(-1)
	}
}

func main() {
	loadg.Verbose = true

	addr := fmt.Sprintf(":%d", setup.LoadgPort)
	server, e := loadg.NewLoadgen(addr)
	epanic(e)

	e = server.Serve()
	epanic(e)
}
