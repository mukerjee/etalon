package loadg

import (
	"bufio"
	"os"
	"strings"
)

func findModule(name string) bool {
	f, e := os.Open("/proc/modules")
	noError(e)
	defer f.Close()

	s := bufio.NewScanner(f)
	for s.Scan() {
		fields := strings.Fields(s.Text())
		if len(fields) == 0 {
			continue
		}

		n := fields[0]
		if n == name {
			return true
		}
	}
	return false
}
