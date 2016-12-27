package react

import "flag"

func parseFlags() {
	flag.IntVar(&nhost, "nhost", nhost, "number of hosts")
	flag.Uint64Var(&weekLen, "week", weekLen, "week length in ticks")
	flag.Uint64Var(&avgDayLen, "avgday", avgDayLen, "day length in ticks")
	flag.Uint64Var(&dayLen, "day", dayLen, "day length in ticks")
	flag.Uint64Var(&nightLen, "night", nightLen, "night length in ticks")
	flag.Uint64Var(&linkBw, "linkbw", linkBw, "end hosts link bandwidth")
	flag.Uint64Var(&packBw, "packbw", packBw, "react packet switch bandwidth")
	flag.Float64Var(&FlowScaling, "flowscale", FlowScaling, "scale of flow load")
	flag.StringVar(&theSwitch, "switch", theSwitch,
		"switch=pack, circ or react")
	flag.Float64Var(&FlowBandwFactor, "flowbandw", FlowBandwFactor,
		"flow bandwidth factor")
	flag.StringVar(&theView, "view", theView,
		"view=console, timeonly or nil")
	flag.StringVar(&theWorkload, "workload", theWorkload,
		"workload=dctcp, test")
	flag.Int64Var(&framePerSec, "fps", framePerSec, "frame per second")
	flag.Uint64Var(&tickPerFrame, "tpf", tickPerFrame,
		"tick per frame, 0 for as fast as possible")
	flag.IntVar(&sosp_small, "small", sosp_small, "mice sizes (sosp only)")

	flag.BoolVar(&sosp_vis, "vis", sosp_vis, "visualize?")

	flag.Parse()
	SetNhost(nhost) // to update nentry
}
