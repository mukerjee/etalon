package main

import (
	"fmt"

	"react/testb/beats"
	"react/testb/setup"
)

func main() {
	beats.Serve(fmt.Sprintf(":%d", setup.BeatsPort))
}
