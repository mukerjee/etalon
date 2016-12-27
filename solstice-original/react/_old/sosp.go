package react

import "time"

import "runtime"
import "os"

var sosp_small = 1
var sosp_nsmall = 4
var sosp_vis = false

func main_sosp() {
	runtime.GOMAXPROCS(2)
	logNoTimestamp = true

	parseFlags()
	if sosp_vis {
		SetNhost(8)
	} else {
		SetNhost(36)
	}
	SetBandw(1250, 250)
	SetTimes(100, 20, 10, 1)

	nicBufCoverage = 100000

	randInit()

	bw := int(float64(linkBw) * 0.95)
	small := sosp_small
	nsmall := sosp_nsmall

	assert(nhost >= nsmall+2)
	big := bw - small*nsmall
	assert(big > 0)
	// logf("small=%d big=%d\n", small, big)

	demandLine := make([]int, 0, nhost)
	demandLine = append(demandLine, 0)
	demandLine = append(demandLine, big)
	for i := 0; i < nsmall; i++ {
		demandLine = append(demandLine, small)
	}

	for len(demandLine) < nhost {
		demandLine = append(demandLine, 0)
	}
	// logln(demandLine)

	hosts := NewDumbHosts(1, demandLine)
	// sender.UseRandom = true

	sw := makeTheSwitch()
	est := NewWindowEstimator()
	testbed := NewTestbed(hosts, sw, est)

	snifferStarts := uint64(1e4)
	testbed.exit.Starts = snifferStarts

	if sosp_vis {
		testbed.View = NewConsoleView()
		testbed.FrameInterval = 50 * time.Millisecond
		testbed.TickPerFrame = 1
	} else {
		testbed.View = NewTimeView()
		testbed.FrameInterval = 500 * time.Millisecond
	}

	ret := testbed.Run(3e4)

	goodput := testbed.Goodput()
	capacity := uint64(nhost) * linkBw * (Time() - snifferStarts)
	eff := float64(goodput) / float64(capacity) * 100
	logf("big=%d,small=%d,eff=%f\n", big, small, eff)

	logf("goodput: %d/%d %.2f%%\n", goodput, capacity,
		float64(goodput)/float64(capacity)*100)

	if ret != 0 {
		os.Exit(ret)
	}
}
