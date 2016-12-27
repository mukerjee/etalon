package react

import (
	"fmt"
	"os"
	"runtime"
	"strings"
	"time"
)

func main1() {
	runtime.GOMAXPROCS(2)

	parseFlags()
	randInit()

	keeper := NewFlowKeeper()
	driver := NewFlowDriver()
	keeper.assignDriver(driver)
	driver.Workload = makeTheWorkload()

	sw := makeTheSwitch()
	est := NewWindowEstimator()

	testbed := NewTestbed(keeper, sw, est)
	testbed.FrameInterval = time.Duration(int64(time.Second) / framePerSec)
	testbed.TickPerFrame = tickPerFrame
	testbed.View = makeTheView()

	ret := testbed.Run(2e6)

	goodput := testbed.Goodput()
	capacity := uint64(nhost) * linkBw * Time()
	fmt.Printf("goodput: %d/%d %.2f%%\n", goodput, capacity,
		float64(goodput)/float64(capacity)*100)

	fmt.Println(driver.tinyFlowSizes, "// tiny flow sizes")
	fmt.Println(driver.smallFlowSizes, "// small flow sizes")
	fmt.Println(driver.bigFlowSizes, "// big flow sizes")

	fmt.Println(driver.tinyFlowTimeCosts, "// tiny flow time costs")
	fmt.Println(driver.smallFlowTimeCosts, "// small flow time costs")
	fmt.Println(driver.bigFlowTimeCosts, "// big flow time costs")
	fmt.Println(driver.queryFlowTimeCosts, "// query flow time costs")
	fmt.Println(driver.queryTimeCosts, "// query complete time")

	fmt.Printf("circuit=%d, packet=%d // tiny flow path\n",
		driver.tinyFlowCircuit, driver.tinyFlowPacket)
	fmt.Printf("circuit=%d, packet=%d // small flow path\n",
		driver.smallFlowCircuit, driver.smallFlowPacket)
	fmt.Printf("circuit=%d, packet=%d // big flow path\n",
		driver.bigFlowCircuit, driver.bigFlowPacket)
	fmt.Printf("circuit=%d, packet=%d // query flow path\n",
		driver.queryFlowCircuit, driver.queryFlowPacket)

	if ret != 0 {
		os.Exit(ret)
	}
}

func main2() {
	runtime.GOMAXPROCS(2)
	SetNhost(8)
	SetBandw(1000, 100)
	SetTimes(300, 20, 10, 0)
	randInit()
	hosts := NewDumbHosts(1, []int{
		480, 480, 5, 5, 5, 5, 5, 5})
	hosts.UseRandom = true
	sw := NewReactSwitch()
	est := NewWindowEstimator()
	testbed := NewTestbed(hosts, sw, est)
	testbed.View = NewConsoleView()
	testbed.FrameInterval = 50 * time.Millisecond
	testbed.TickPerFrame = 1

	testbed.Run(1e5)

	goodput := testbed.Goodput()
	capacity := uint64(nhost) * linkBw * Time()
	fmt.Printf("goodput: %d/%d %.2f%%\n", goodput, capacity,
		float64(goodput)/float64(capacity)*100)
}

func main3() {
	SetNhost(3)
	SetBandw(1000, 100)
	// dayLen = 10
	// weekLen = 300
	nightLen = 0

	sched := NewScheduler()
	sched.DoInterleave = true
	sched.DoTrim = false

	demand := Matrix{4, 9, 2, 3, 5, 7, 8, 1, 6}
	demand.mul(weekLen * linkBw / 15)
	days := sched.Schedule(demand, weekLen)

	demand.div(weekLen)
	logln(demand, "// demand")

	for i, d := range days {
		logln(d, fmt.Sprintf("// day %d", i+1))
	}

	logln(sched.realBandw, "// real bandw")
	logln(sched.tickCount, "// tick count")
}

func main4() {
	SetNhost(2)
	finder := NewMapFinder()
	m := Matrix{1, 0, 7, 5}
	nmatch, res := finder.FindMax(m)
	logln(m)
	logln(nmatch, res)
}

func main5() {
	SetNhost(8)
	reader := strings.NewReader(`
	100us {
		- 100 100 100
		100 - 100 100
		100 100 - 100
		100 100 100 -
	}

	200us {
		- 30% 30% 30%
		30% - 30% 30%
		30% 30% - 30%
		30% 30% 30% -
	}

	0.15ms {
		-
	}

	repeat 3
	`)
	demand, errors := ParseDemand(reader)
	if errors != nil {
		for _, e := range errors {
			fmt.Println(e)
		}
	} else {
		fmt.Println(demand.String())
		s := demand.String()

		// parse it again
		demand, errors = ParseDemand(strings.NewReader(s))
		fmt.Println(demand.String())
	}
}

func main() {
	main_sosp()
}

func Main() {
	main_nsdi()

	// main_sosp()
	// main5()
}
