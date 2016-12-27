package demand

import (
	"bufio"
	"errors"
	"io"
	"os"
	"strconv"
	"strings"
)

func Read(r io.Reader) (*Demand, error) {
	var ret *Demand
	header := new(header)
	s := bufio.NewScanner(r)
	var p *Period
	var e error
	row := 0
	nhost := 0

	for s.Scan() {
		line := strings.TrimSpace(s.Text())

		switch {
		case line == "":
		case strings.HasPrefix(line, "%"):
		/* do nothing */

		case strings.HasPrefix(line, "#"):
			header.Name = strings.TrimSpace(line[1:])

		case strings.HasPrefix(line, "//"):
			header.Memo = strings.TrimSpace(line[2:])

		case strings.HasPrefix(line, "n="):
			nhost, e = strconv.Atoi(strings.TrimSpace(line[2:]))
			if e != nil {
				return nil, e
			}
			if nhost <= 1 {
				return nil, errors.New("nhost needs at least 2")
			}
			ret = NewDemand(nhost)

		case strings.HasPrefix(line, "t="):
			if ret == nil {
				return nil, errors.New("nhost line missing")
			}
			n, e := strconv.Atoi(strings.TrimSpace(line[2:]))
			if e != nil {
				return nil, e
			}
			if n <= 0 {
				return nil, errors.New("time must be positive")
			}

			p = NewPeriod(nhost, uint64(n))
			ret.Add(p)

			row = 0

		default:
			if ret == nil {
				return nil, errors.New("nhost line missing")
			}
			if p == nil {
				return nil, errors.New("time not specified")
			}
			if row >= nhost {
				return nil, errors.New("too many rows in a period")
			}

			fields := strings.Fields(line)
			if len(fields) > nhost {
				return nil, errors.New("too many fields in a row")
			}

			for i, f := range fields {
				r := 0
				if f != "-" {
					r, e = strconv.Atoi(f)
				}
				if r < 0 {
					return nil, errors.New("rate must be non negative")
				}

				if row == i && r != 0 {
					return nil, errors.New("can not send to self")
				}

				p.D[row][i] = uint64(r)
			}

			row++
		}
	}

	e = s.Err()
	if e != nil {
		return nil, e
	}

	if ret == nil {
		panic("bug")
	}

	return ret, nil
}

func Load(path string) (*Demand, error) {
	f, e := os.Open(path)
	if e != nil {
		return nil, e
	}
	defer f.Close()

	return Read(f)
}

func Parse(s string) (*Demand, error) {
	return Read(strings.NewReader(s))
}
