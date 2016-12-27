package main

// mapping from lane to portId on the mindspeed switch
// (for mapping from source to destination, see match.go)
type PortMap map[string]uint8

func isValidLocation(s string) bool {
	return inPorts[s] != nil
}

func isValidLane(s string) bool {
	_, found := inPorts["se"][s]
	return found
}

var inPorts = map[string]PortMap{
	"se": {
		"d0": 23, "d1": 21, "d2": 22, "d3": 20,
		"e0": 12, "e1": 14, "e2": 13, "e3": 15,
		"f0": 29, "f1": 28, "f2": 31, "f3": 30,
	},
	"sw": {
		"d0": 47, "d1": 45, "d2": 46, "d3": 44,
		"e0": 36, "e1": 38, "e2": 37, "e3": 39,
		"f0": 5, "f1": 4, "f2": 7, "f3": 6,
	},
	"ne": {
		"d0": 35, "d1": 33, "d2": 34, "d3": 32,
		"e0": 40, "e1": 42, "e2": 41, "e3": 43,
		"f0": 17, "f1": 16, "f2": 19, "f3": 18,
	},
	"nw": {
		"d0": 11, "d1": 9, "d2": 10, "d3": 8,
		"e0": 0, "e1": 2, "e2": 1, "e3": 3,
		"f0": 25, "f1": 24, "f2": 27, "f3": 26,
	},
}

var outPorts = map[string]PortMap{
	"se": {
		"d0": 47, "d1": 45, "d2": 46, "d3": 44,
		"e0": 36, "e1": 38, "e2": 37, "e3": 39,
		"f0": 5, "f1": 4, "f2": 7, "f3": 6,
	},
	"sw": {
		"d0": 23, "d1": 21, "d2": 22, "d3": 20,
		"e0": 12, "e1": 14, "e2": 13, "e3": 15,
		"f0": 29, "f1": 28, "f2": 31, "f3": 30,
	},
	"ne": {
		"d0": 11, "d1": 9, "d2": 10, "d3": 8,
		"e0": 0, "e1": 2, "e2": 1, "e3": 3,
		"f0": 25, "f1": 24, "f2": 27, "f3": 26,
	},
	"nw": {
		"d0": 35, "d1": 33, "d2": 34, "d3": 32,
		"e0": 40, "e1": 42, "e2": 41, "e3": 43,
		"f0": 17, "f1": 16, "f2": 19, "f3": 18,
	},
}
