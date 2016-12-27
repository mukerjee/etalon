package main

import (
	"fmt"
	"react/conf"
)

func main() {
	conf.Load()
	fmt.Println(conf.String())
}
