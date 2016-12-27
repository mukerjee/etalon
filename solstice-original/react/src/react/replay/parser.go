package replay

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

func Parse(s string) (*Replay, error) {
	reader := strings.NewReader(s)
	return ReadFrom(reader)
}

func Load(path string) (*Replay, error) {
	f, e := os.Open(path)
	if e != nil {
		return nil, e
	}
	defer f.Close()
	return ReadFrom(f)
}

func ReadFrom(reader io.Reader) (*Replay, error) {
	var ret *Replay
	var tick *Tick
	var e error
	scanner := bufio.NewScanner(reader)
	name := ""
	memo := ""
	timeExpect := uint64(0)
	nchan := 0
	chanPres := make([]string, 0, 10)
	pre := ""

	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		switch {
		case line == "":
			// do nothing

		case strings.HasPrefix(line, "#"):
			name = strings.TrimSpace(line[1:])
			if ret != nil {
				ret.Name = name
			}

		case strings.HasPrefix(line, "//"):
			memo = strings.TrimSpace(line[2:])
			if ret != nil {
				ret.Memo = memo
			}

		case strings.HasPrefix(line, "n="):
			nhost, e := strconv.Atoi(strings.TrimSpace(line[2:]))
			if e != nil {
				return nil, fmt.Errorf("error parsing nhost: %s", e)
			}
			if nhost <= 1 {
				return nil, errors.New("invalid nhost")
			}
			ret = NewReplay(nhost)
			ret.Name = name
			ret.Memo = memo
			ret.LinkBw = 1250
			ret.PackBw = 125

		case strings.HasPrefix(line, "linkbw="):
			if ret == nil {
				return nil, errors.New("linkbw must be after nhost")
			}
			linkbw, e := strconv.Atoi(strings.TrimSpace(line[7:]))
			if e != nil {
				return nil, fmt.Errorf("error parsing linkbw: %s", e)
			}
			if linkbw <= 0 {
				return nil, errors.New("invalid linkbw")
			}
			ret.LinkBw = uint64(linkbw)

		case strings.HasPrefix(line, "packbw="):
			if ret == nil {
				return nil, errors.New("packbw must be after nhost")
			}
			packbw, e := strconv.Atoi(strings.TrimSpace(line[7:]))
			if e != nil {
				return nil, fmt.Errorf("error parsing packbw: %s", e)
			}
			if packbw <= 0 {
				return nil, errors.New("invalid packbw")
			}
			ret.PackBw = uint64(packbw)

		case strings.HasPrefix(line, "chans="):
			chans := strings.Split(line[6:], ",")
			nchan = len(chans)
			if nchan == 0 {
				return nil, errors.New("no channel")
			}
			for _, c := range chans {
				c = strings.TrimSpace(c)
				if c == "" {
					return nil, errors.New("empty chan name")
				} else {
					if !ret.addChan(c) {
						return nil, errors.New("duplicated chan")
					}
					chanPres = append(chanPres, c+":")
				}
			}

		case strings.HasPrefix(line, "t="):
			if nchan == 0 {
				return nil, errors.New("chans missing")
			}
			tagsIndex := strings.Index(line, "//")
			var timeGot uint64
			var timePart, tagsPart string

			if tagsIndex < 0 {
				timePart = line[2:]
			} else {
				timePart = line[2:tagsIndex]
				tagsPart = line[tagsIndex+2:]
			}

			timePart = strings.TrimSpace(timePart)
			tagsPart = strings.TrimSpace(tagsPart)

			t, e := strconv.Atoi(timePart)
			if e != nil {
				return nil, fmt.Errorf("error parsing time: %s", e)
			}

			if uint64(t) != timeExpect {
				return nil, fmt.Errorf("time expect %d, got %d",
					timeExpect, timeGot)
			}
			timeExpect++

			tick = NewTick(nchan)
			if tagsPart != "" {
				tags := strings.Split(tagsPart, ",")
				for _, tag := range tags {
					tag = strings.TrimSpace(tag)
					if tag == "" {
						return nil, errors.New("empty tag")
					}
					tick.Tag(tag)
				}
			}
			pre = chanPres[0]

		case tick != nil && strings.HasPrefix(line, pre):
			m, e := parseMatrix(strings.TrimSpace(line[len(pre):]), ret.Nhost())
			if e != nil {
				return nil, e
			}

			tickNchan := tick.Chan(m)
			if tickNchan == nchan {
				t := ret.addTick(tick)
				if t != timeExpect {
					panic("buf")
				}

				tick = nil
			} else {
				pre = chanPres[tickNchan]
			}

		default:
			return nil, fmt.Errorf("invalid line: %s", line)
		}
	}

	e = scanner.Err()
	if e != nil {
		return nil, e
	}

	return ret, nil
}

func parseNumber(s string) (uint64, error) {
	if s == "-" {
		return 0, nil
	}
	n, e := strconv.Atoi(s)
	if e != nil {
		return 0, e
	}
	return uint64(n), nil
}

func parseMatrix(line string, nhost int) (Matrix, error) {
	rows := strings.Split(line, ";")
	if len(rows) != nhost {
		return nil, fmt.Errorf("invalid matrix: %s", line)
	}

	ret := NewMatrix(nhost)
	for i, r := range rows {
		cols := strings.Fields(r)
		if len(cols) != nhost {
			return nil, fmt.Errorf("invalid matrix: %s", line)
		}
		base := i * nhost
		for j, c := range cols {
			n, e := parseNumber(c)
			if e != nil {
				return nil, fmt.Errorf("invalid Matrix: %s", line)
			}
			ret[base+j] = n
		}
	}

	return ret, nil
}
