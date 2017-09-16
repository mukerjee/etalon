define($DEVNAME eth2)

define($NUMHOSTS 8)
define($IP11 10.10.1.11, $IP12 10.10.1.12, $IP13 10.10.1.13, $IP14 10.10.1.14,
       $IP15 10.10.1.15, $IP16 10.10.1.16, $IP17 10.10.1.17, $IP18 10.10.1.18,
       $IP21 10.10.1.21, $IP22 10.10.1.22, $IP23 10.10.1.23, $IP24 10.10.1.24,
       $IP25 10.10.1.25, $IP26 10.10.1.26, $IP27 10.10.1.27, $IP28 10.10.1.28,
       $IP31 10.10.1.31, $IP32 10.10.1.32, $IP33 10.10.1.33, $IP34 10.10.1.34,
       $IP35 10.10.1.35, $IP36 10.10.1.36, $IP37 10.10.1.37, $IP38 10.10.1.38,
       $IP41 10.10.1.41, $IP42 10.10.1.42, $IP43 10.10.1.43, $IP44 10.10.1.44,
       $IP45 10.10.1.45, $IP46 10.10.1.46, $IP47 10.10.1.47, $IP48 10.10.1.48,
       $IP51 10.10.1.51, $IP52 10.10.1.52, $IP53 10.10.1.53, $IP54 10.10.1.54,
       $IP55 10.10.1.55, $IP56 10.10.1.56, $IP57 10.10.1.57, $IP58 10.10.1.58,
       $IP61 10.10.1.61, $IP62 10.10.1.62, $IP63 10.10.1.63, $IP64 10.10.1.64,
       $IP65 10.10.1.65, $IP66 10.10.1.66, $IP67 10.10.1.67, $IP68 10.10.1.68,
       $IP71 10.10.1.71, $IP72 10.10.1.72, $IP73 10.10.1.73, $IP74 10.10.1.74,
       $IP75 10.10.1.75, $IP76 10.10.1.76, $IP77 10.10.1.77, $IP78 10.10.1.78,
       $IP81 10.10.1.81, $IP82 10.10.1.82, $IP83 10.10.1.83, $IP84 10.10.1.84,
       $IP85 10.10.1.85, $IP86 10.10.1.86, $IP87 10.10.1.87, $IP88 10.10.1.88)

// define($IP0 10.10.1.2, $IP1 10.10.1.3, $IP2 10.10.1.4, $IP3 10.10.1.5)

define ($CIRCUIT_BW 4Gbps, $PACKET_BW 0.5Gbps)
// define ($RTT 60)  // usecs -- measured
// define ($MTU 9000)  // bytes
// define ($DELAY_LATENCY 0.000140) // seconds 10 - (RTT / TDF) * TDF, if your target is 10us
// define ($DELAY_LATENCY 0) // Not worrying about this-- host-to-host ends up being ~15us without delaying

// TODO emperical
define ($BIG_BUFFER_SIZE 1000)
define ($SMALL_BUFFER_SIZE 100)

define ($RECONFIG_DELAY 20)  // usecs
define ($TDF 20)

StaticThreadSched(in 0,
		  traffic_matrix 1,
		  sol 2,
		  runner 3,
                  hybrid_switch/circuit_link0 4,
                  hybrid_switch/circuit_link1 4,
                  hybrid_switch/circuit_link2 4,
                  hybrid_switch/circuit_link3 4,
                  hybrid_switch/circuit_link4 5,
                  hybrid_switch/circuit_link5 5,
                  hybrid_switch/circuit_link6 5,
                  hybrid_switch/circuit_link7 5,
		  hybrid_switch/packet_up_link0 6,
                  hybrid_switch/packet_up_link1 6,
                  hybrid_switch/packet_up_link2 6,
                  hybrid_switch/packet_up_link3 6,
		  hybrid_switch/packet_up_link4 6,
                  hybrid_switch/packet_up_link5 6,
                  hybrid_switch/packet_up_link6 6,
                  hybrid_switch/packet_up_link7 6,
		  hybrid_switch/ps/packet_link0 6,
		  hybrid_switch/ps/packet_link1 6,
		  hybrid_switch/ps/packet_link2 6,
		  hybrid_switch/ps/packet_link3 6,
		  hybrid_switch/ps/packet_link4 6,
		  hybrid_switch/ps/packet_link5 6,
		  hybrid_switch/ps/packet_link6 6,
		  hybrid_switch/ps/packet_link7 6,
		  )

ControlSocket("TCP", 1239)

traffic_matrix :: EstimateTraffic($NUMHOSTS, SOURCE QUEUE)
sol :: Solstice($NUMHOSTS, $CIRCUIT_BW, $PACKET_BW, $RECONFIG_DELAY, $TDF)
runner :: RunSchedule($NUMHOSTS, $BIG_BUFFER_SIZE, $SMALL_BUFFER_SIZE, RESIZE false)

// Script(write runner.setSchedule 2 2000000 1/0/2/3/4/5/6/7 2000000 -1/-1/-1/-1/-1/-1/-1/-1)
Script(wait 1, print hybrid_switch/q01/q.capacity, loop)
// Script(wait 10, write runner.setDoResize true)
// Script(wait 10, write traffic_matrix.setSource ADU)

in :: FromDPDKDevice(0)
out :: ToDPDKDevice(0)

arp_c :: Classifier(12/0800, 12/0806 20/0002, 12/0806 20/0001)
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth)
arp_r :: ARPResponder($DEVNAME)

elementclass in_classfy {
    input[0] -> IPClassifier(
  src host $IP11 or src host $IP12 or src host $IP13 or src host $IP14 or
  src host $IP15 or src host $IP16 or src host $IP17 or src host $IP18,
  src host $IP21 or src host $IP22 or src host $IP23 or src host $IP24 or
  src host $IP25 or src host $IP26 or src host $IP27 or src host $IP28,
  src host $IP31 or src host $IP32 or src host $IP33 or src host $IP34 or
  src host $IP35 or src host $IP36 or src host $IP37 or src host $IP38,
  src host $IP41 or src host $IP42 or src host $IP43 or src host $IP44 or
  src host $IP45 or src host $IP46 or src host $IP47 or src host $IP48,
  src host $IP51 or src host $IP52 or src host $IP53 or src host $IP54 or
  src host $IP55 or src host $IP56 or src host $IP57 or src host $IP58,
  src host $IP61 or src host $IP62 or src host $IP63 or src host $IP64 or
  src host $IP65 or src host $IP66 or src host $IP67 or src host $IP68,
  src host $IP71 or src host $IP72 or src host $IP73 or src host $IP74 or
  src host $IP75 or src host $IP76 or src host $IP77 or src host $IP78,
  src host $IP81 or src host $IP82 or src host $IP83 or src host $IP84 or
  src host $IP85 or src host $IP86 or src host $IP87 or src host $IP88
)
             => [0, 1, 2, 3, 4, 5, 6, 7]output
}

elementclass out_classfy {
    input[0] -> IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
)
             => [0, 1, 2, 3, 4, 5, 6, 7]output
}

elementclass packet_link {
    input[0,1,2,3,4,5,6,7] 
                           => RoundRobinSched 
			   -> BandwidthRatedUnqueue($PACKET_BW)
			   -> output
}

elementclass circuit_link {
    input[0,1,2,3,4,5,6,7] 
                           => ps :: PullSwitch(-1)
                           -> BandwidthRatedUnqueue($CIRCUIT_BW)
		           -> output
}

elementclass packet_switch {
    c0, c1, c2, c3, c4, c5, c6, c7 :: out_classfy

    q00, q01, q02, q03, q04, q05, q06, q07,
    q10, q11, q12, q13, q14, q15, q16, q17,
    q20, q21, q22, q23, q24, q25, q26, q27,
    q30, q31, q32, q33, q34, q35, q36, q37,
    q40, q41, q42, q43, q44, q45, q46, q47,
    q50, q51, q52, q53, q54, q55, q56, q57,
    q60, q61, q62, q63, q64, q65, q66, q67,
    q70, q71, q72, q73, q74, q75, q76, q77
 :: Queue(CAPACITY 1)

    packet_link0, packet_link1, packet_link2, packet_link3,
    packet_link4, packet_link5, packet_link6, packet_link7  :: packet_link

    input[0] -> c0 => q00, q01, q02, q03, q04, q05, q06, q07
    input[1] -> c1 => q10, q11, q12, q13, q14, q15, q16, q17
    input[2] -> c2 => q20, q21, q22, q23, q24, q25, q26, q27
    input[3] -> c3 => q30, q31, q32, q33, q34, q35, q36, q37
    input[4] -> c4 => q40, q41, q42, q43, q44, q45, q46, q47
    input[5] -> c5 => q50, q51, q52, q53, q54, q55, q56, q57
    input[6] -> c6 => q60, q61, q62, q63, q64, q65, q66, q67
    input[7] -> c7 => q70, q71, q72, q73, q74, q75, q76, q77

    q00, q10, q20, q30, q40, q50, q60, q70 => packet_link0 -> [0]output
    q01, q11, q21, q31, q41, q51, q61, q71 => packet_link1 -> [1]output
    q02, q12, q22, q32, q42, q52, q62, q72 => packet_link2 -> [2]output
    q03, q13, q23, q33, q43, q53, q63, q73 => packet_link3 -> [3]output
    q04, q14, q24, q34, q44, q54, q64, q74 => packet_link4 -> [4]output
    q05, q15, q25, q35, q45, q55, q65, q75 => packet_link5 -> [5]output
    q06, q16, q26, q36, q46, q56, q66, q76 => packet_link6 -> [6]output
    q07, q17, q27, q37, q47, q57, q67, q77 => packet_link7 -> [7]output

    // would be drops
    q00[1], q01[1], q02[1], q03[1], q04[1], q05[1], q06[1], q07[1] -> [8]output
    q10[1], q11[1], q12[1], q13[1], q14[1], q15[1], q16[1], q17[1] -> [9]output
    q20[1], q21[1], q22[1], q23[1], q24[1], q25[1], q26[1], q27[1] -> [10]output
    q30[1], q31[1], q32[1], q33[1], q34[1], q35[1], q36[1], q37[1] -> [11]output
    q40[1], q41[1], q42[1], q43[1], q44[1], q45[1], q46[1], q47[1] -> [12]output
    q50[1], q51[1], q52[1], q53[1], q54[1], q55[1], q56[1], q57[1] -> [13]output
    q60[1], q61[1], q62[1], q63[1], q64[1], q65[1], q66[1], q67[1] -> [14]output
    q70[1], q71[1], q72[1], q73[1], q74[1], q75[1], q76[1], q77[1] -> [15]output
}

hybrid_switch :: {
    c0, c1, c2, c3, c4, c5, c6, c7 :: out_classfy

    // ToR queues (in here for convenience)
    q00, q01, q02, q03, q04, q05, q06, q07,
    q10, q11, q12, q13, q14, q15, q16, q17,
    q20, q21, q22, q23, q24, q25, q26, q27,
    q30, q31, q32, q33, q34, q35, q36, q37,
    q40, q41, q42, q43, q44, q45, q46, q47,
    q50, q51, q52, q53, q54, q55, q56, q57,
    q60, q61, q62, q63, q64, q65, q66, q67,
    q70, q71, q72, q73, q74, q75, q76, q77
 // :: Queue(CAPACITY $SMALL_BUFFER_SIZE)
 :: {
        input[0] -> q ::Queue(CAPACITY $SMALL_BUFFER_SIZE)
	input[1] -> lq :: Queue(CAPACITY 1) // loss queue
	lq, q => PrioSched -> output
    }

    circuit_link0, circuit_link1, circuit_link2, circuit_link3,
    circuit_link4, circuit_link5, circuit_link6, circuit_link7 :: circuit_link

    packet_up_link0, packet_up_link1, packet_up_link2, packet_up_link3,
    packet_up_link4, packet_up_link5, packet_up_link6, packet_up_link7 :: packet_link

    ps :: packet_switch

    input[0] -> c0 => q00, q01, q02, q03, q04, q05, q06, q07
    input[1] -> c1 => q10, q11, q12, q13, q14, q15, q16, q17
    input[2] -> c2 => q20, q21, q22, q23, q24, q25, q26, q27
    input[3] -> c3 => q30, q31, q32, q33, q34, q35, q36, q37
    input[4] -> c4 => q40, q41, q42, q43, q44, q45, q46, q47
    input[5] -> c5 => q50, q51, q52, q53, q54, q55, q56, q57
    input[6] -> c6 => q60, q61, q62, q63, q64, q65, q66, q67
    input[7] -> c7 => q70, q71, q72, q73, q74, q75, q76, q77

    q00, q10, q20, q30, q40, q50, q60, q70 => circuit_link0 -> [0]output
    q01, q11, q21, q31, q41, q51, q61, q71 => circuit_link1 -> [1]output
    q02, q12, q22, q32, q42, q52, q62, q72 => circuit_link2 -> [2]output
    q03, q13, q23, q33, q43, q53, q63, q73 => circuit_link3 -> [3]output
    q04, q14, q24, q34, q44, q54, q64, q74 => circuit_link4 -> [4]output
    q05, q15, q25, q35, q45, q55, q65, q75 => circuit_link5 -> [5]output
    q06, q16, q26, q36, q46, q56, q66, q76 => circuit_link6 -> [6]output
    q07, q17, q27, q37, q47, q57, q67, q77 => circuit_link7 -> [7]output

    q00, q01, q02, q03, q04, q05, q06, q07 => packet_up_link0 -> [0]ps[0] -> [0]output
    q10, q11, q12, q13, q14, q15, q16, q17 => packet_up_link1 -> [1]ps[1] -> [1]output
    q20, q21, q22, q23, q24, q25, q26, q27 => packet_up_link2 -> [2]ps[2] -> [2]output
    q30, q31, q32, q33, q34, q35, q36, q37 => packet_up_link3 -> [3]ps[3] -> [3]output
    q40, q41, q42, q43, q44, q45, q46, q47 => packet_up_link4 -> [4]ps[4] -> [4]output
    q50, q51, q52, q53, q54, q55, q56, q57 => packet_up_link5 -> [5]ps[5] -> [5]output
    q60, q61, q62, q63, q64, q65, q66, q67 => packet_up_link6 -> [6]ps[6] -> [6]output
    q70, q71, q72, q73, q74, q75, q76, q77 => packet_up_link7 -> [7]ps[7] -> [7]output

    // dropped PS packets -> loss queues
    ps[ 8] -> out_classfy => [1]q00, [1]q01, [1]q02, [1]q03, [1]q04, [1]q05, [1]q06, [1]q07
    ps[ 9] -> out_classfy => [1]q10, [1]q11, [1]q12, [1]q13, [1]q14, [1]q15, [1]q16, [1]q17
    ps[10] -> out_classfy => [1]q20, [1]q21, [1]q22, [1]q23, [1]q24, [1]q25, [1]q26, [1]q27
    ps[11] -> out_classfy => [1]q30, [1]q31, [1]q32, [1]q33, [1]q34, [1]q35, [1]q36, [1]q37
    ps[12] -> out_classfy => [1]q40, [1]q41, [1]q42, [1]q43, [1]q44, [1]q45, [1]q46, [1]q47
    ps[13] -> out_classfy => [1]q50, [1]q51, [1]q52, [1]q53, [1]q54, [1]q55, [1]q56, [1]q57
    ps[14] -> out_classfy => [1]q60, [1]q61, [1]q62, [1]q63, [1]q64, [1]q65, [1]q66, [1]q67
    ps[15] -> out_classfy => [1]q70, [1]q71, [1]q72, [1]q73, [1]q74, [1]q75, [1]q76, [1]q77
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16) 
   -> pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -)[1]
   -> in_classfy[0,1,2,3,4,5,6,7]
   => hybrid_switch[0,1,2,3,4,5,6,7]
   -> arp -> out

arp_c[1] -> [1]arp
arp_c[2] -> arp_r -> out

pc -> ICMPPingResponder -> arp
