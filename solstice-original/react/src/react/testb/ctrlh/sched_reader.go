package ctrlh

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"

	"react/testb/setup"
)

func parseDay(line string) (*SchedDay, error) {
	fields := strings.Fields(line)
	if len(fields) < 2 {
		return nil, fmt.Errorf("missing fields: %s", line)
	}

	dstr := fields[0]
	if dstr[0] != 'd' {
		panic("dstr not started with a d")
	}
	ret := new(SchedDay)
	var e error
	ret.Index, e = strconv.Atoi(dstr[1:])
	if e != nil {
		return nil, e
	}

	tstr := fields[1]
	tstrLen := len(tstr)
	if tstr[tstrLen-1] != ':' {
		return nil, fmt.Errorf("invalid time field: %s", line)
	}
	n, e := strconv.Atoi(tstr[:tstrLen-1])
	if e != nil {
		return nil, e
	}
	if n <= 0 {
		return nil, fmt.Errorf("invalid time field: %s", line)
	}
	ret.Len = uint64(n)

	lanes := fields[2:]

	ret.Lanes = make([]*SchedLane, 0, setup.Nhost)
	for _, laneStr := range lanes {
		parts := strings.Split(laneStr, "/")
		if len(parts) != 2 {
			return nil, fmt.Errorf("invalid lanes: %s", line)
		}
		rate, e := strconv.Atoi(parts[1])
		if e != nil {
			return nil, e
		}
		if rate < 0 || rate > 100 {
			return nil, fmt.Errorf("invalid rate: %s", line)
		}

		ports := strings.Split(parts[0], "-")
		if len(ports) != 2 {
			return nil, fmt.Errorf("invalid lane: %s", line)
		}
		src, e := strconv.Atoi(ports[0])
		if e != nil {
			return nil, e
		}
		if src < 0 || src >= setup.Nhost {
			return nil, fmt.Errorf("invalid lane: %s", line)
		}
		dest, e := strconv.Atoi(ports[1])
		if e != nil {
			return nil, e
		}
		if dest < 0 || dest >= setup.Nhost {
			return nil, fmt.Errorf("invalid lane: %s", line)
		}

		lane := &SchedLane{
			Src:  src,
			Dest: dest,
			Rate: rate,
		}

		ret.Lanes = append(ret.Lanes, lane)
	}

	if !ret.Check() {
		return nil, fmt.Errorf("invalid mapping: %s", line)
	}
	return ret, nil
}

func ReadSchedule(r io.Reader) (*Schedule, error) {
	scanner := bufio.NewScanner(r)
	ret := newSchedule()
	var week *SchedWeek

	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())

		switch {
		case line == "":
		case strings.HasPrefix(line, "n="):
			n, e := strconv.Atoi(line[2:])
			if e != nil {
				return nil, fmt.Errorf("invalid nhost: %s", e)
			}

			if n != ret.Nhost {
				return nil, errors.New("nhost mismatch")
			}
		case strings.HasPrefix(line, "linkbw="):
			n, e := strconv.Atoi(line[7:])
			if e != nil {
				return nil, fmt.Errorf("invalid linkbw: %s", e)
			}
			if n < 0 || uint64(n) != ret.LinkBw {
				return nil, errors.New("linkbw mismatch")
			}
		case strings.HasPrefix(line, "packbw="):
			n, e := strconv.Atoi(line[7:])
			if e != nil {
				return nil, fmt.Errorf("invalid packbw: %s", e)
			}
			if n < 0 || uint64(n) != ret.PackBw {
				return nil, errors.New("packbw mismatch")
			}
		case strings.HasPrefix(line, "// T="):
			week = newWeek(len(ret.Weeks))
			ret.Weeks = append(ret.Weeks, week)
		case strings.HasPrefix(line, "d"):
			if week == nil {
				return nil, errors.New("day before a week declaration")
			}
			day, e := parseDay(line)
			if e != nil {
				return nil, e
			}
			if day.Index != len(week.Days) {
				return nil, errors.New("invalid day index")
			}
			week.Days = append(week.Days, day)
		default:
			return nil, fmt.Errorf("invalid line: %s", line)
		}
	}

	e := scanner.Err()
	if e != nil {
		return nil, e
	}

	return ret, nil
}

func LoadSchedule(path string) (*Schedule, error) {
	f, e := os.Open(path)
	if e != nil {
		return nil, e
	}
	defer f.Close()
	return ReadSchedule(f)
}
