package main

type Comm interface {
	Send(p []byte)
}
