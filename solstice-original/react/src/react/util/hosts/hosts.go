package hosts

import (
	"bufio"
	"bytes"
	"io"
	"os"
	"strings"
)

type Host struct {
	Name string
	IP   string
}

func NewHost(ip string) *Host {
	assert(IsIP(ip))
	return &Host{ip, ip}
}

func NewNamedHost(ip, name string) *Host {
	assert(IsIP(ip))
	return &Host{name, ip}
}

func (self *Host) IsSelf() bool {
	return IsSelf(self.IP)
}

type Hosts []*Host

type Reader interface {
	ReadString(delim byte) (line string, err error)
}

func OpenFile(path string) Hosts {
	file, err := os.Open(path)
	noErr(err)
	reader := bufio.NewReader(file)
	ret := newHosts(reader)

	file.Close()

	return ret
}

func Parse(s string) Hosts {
	buf := bytes.NewBufferString(s)
	return newHosts(buf)
}

func parseLine(line string) *Host {
	fields := strings.Fields(line)
	assert(len(fields) > 0)

	if len(fields) == 1 {
		return NewHost(line)
	}
	return NewNamedHost(fields[0], fields[1])
}

func newHosts(reader Reader) Hosts {
	ret := make([]*Host, 0, 8)

	for {
		line, err := reader.ReadString('\n')
		if err == io.EOF {
			break
		}
		noErr(err)

		line = strings.TrimSpace(line)

		if len(line) == 0 {
			continue // blank line
		}
		if strings.HasPrefix(line, "//") {
			continue // comment line
		}
		if strings.HasPrefix(line, "#") {
			continue // comment line
		}

		ret = append(ret, parseLine(line))
	}

	return ret
}
