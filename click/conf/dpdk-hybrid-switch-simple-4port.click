define($DEVNAME eth2)

define($NUMHOSTS 4)
define($IP11 10.10.1.11, $IP12 10.10.1.12, $IP13 10.10.1.13, $IP14 10.10.1.14,
       $IP15 10.10.1.15, $IP16 10.10.1.16, $IP17 10.10.1.17, $IP18 10.10.1.18,
       $IP21 10.10.1.21, $IP22 10.10.1.22, $IP23 10.10.1.23, $IP24 10.10.1.24,
       $IP25 10.10.1.25, $IP26 10.10.1.26, $IP27 10.10.1.27, $IP28 10.10.1.28,
       $IP31 10.10.1.31, $IP32 10.10.1.32, $IP33 10.10.1.33, $IP34 10.10.1.34,
       $IP35 10.10.1.35, $IP36 10.10.1.36, $IP37 10.10.1.37, $IP38 10.10.1.38,
       $IP41 10.10.1.41, $IP42 10.10.1.42, $IP43 10.10.1.43, $IP44 10.10.1.44,
       $IP45 10.10.1.45, $IP46 10.10.1.46, $IP47 10.10.1.47, $IP48 10.10.1.48)

define ($CIRCUIT_BW 4Gbps, $PACKET_BW 0.5Gbps)

// TODO emperical
define ($BIG_BUFFER_SIZE 100)
define ($SMALL_BUFFER_SIZE 10)

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
		  hybrid_switch/packet_up_link0 6,
                  hybrid_switch/packet_up_link1 6,
                  hybrid_switch/packet_up_link2 7,
                  hybrid_switch/packet_up_link3 7,
		  hybrid_switch/ps/packet_link0 6,
		  hybrid_switch/ps/packet_link1 6,
		  hybrid_switch/ps/packet_link2 7,
		  hybrid_switch/ps/packet_link3 7,
		  )

ControlSocket("TCP", 1239)

traffic_matrix :: EstimateTraffic($NUMHOSTS, SOURCE QUEUE)
sol :: Solstice($NUMHOSTS, $CIRCUIT_BW, $PACKET_BW, $RECONFIG_DELAY, $TDF)
runner :: RunSchedule($NUMHOSTS, $BIG_BUFFER_SIZE, $SMALL_BUFFER_SIZE, RESIZE false)
// Script(write runner.setSchedule 1 2000000 -1/-1/-1/-1)

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
)
             => [0, 1, 2, 3]output
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
)
             => [0, 1, 2, 3]output
}

elementclass packet_link {
    input[0,1,2,3] 
                           => RoundRobinSched 
                  	   -> BandwidthRatedUnqueue($PACKET_BW)
			   -> output
}

elementclass circuit_link {
    input[0,1,2,3] 
                           => ps :: PullSwitch(-1)
                           -> BandwidthRatedUnqueue($CIRCUIT_BW)
		           -> output
}

elementclass packet_switch {
    c0, c1, c2, c3 :: out_classfy

    q00, q01, q02, q03, 
    q10, q11, q12, q13, 
    q20, q21, q22, q23, 
    q30, q31, q32, q33 :: Queue(CAPACITY 1)

    packet_link0, packet_link1, packet_link2, packet_link3 :: packet_link

    input[0] -> c0 => q00, q01, q02, q03
    input[1] -> c1 => q10, q11, q12, q13
    input[2] -> c2 => q20, q21, q22, q23 
    input[3] -> c3 => q30, q31, q32, q33 

    q00, q10, q20, q30 => packet_link0 -> [0]output
    q01, q11, q21, q31 => packet_link1 -> [1]output
    q02, q12, q22, q32 => packet_link2 -> [2]output
    q03, q13, q23, q33 => packet_link3 -> [3]output
}

hybrid_switch :: {
    c0, c1, c2, c3  :: out_classfy

    // ToR queues (in here for convenience)
    q00, q01, q02, q03, 
    q10, q11, q12, q13, 
    q20, q21, q22, q23, 
    q30, q31, q32, q33  :: Queue(CAPACITY 100)

    circuit_link0, circuit_link1, circuit_link2, circuit_link3  :: circuit_link

    packet_up_link0, packet_up_link1, packet_up_link2, packet_up_link3 :: packet_link

    ps :: packet_switch

    input[0] -> c0 => q00, q01, q02, q03
    input[1] -> c1 => q10, q11, q12, q13
    input[2] -> c2 => q20, q21, q22, q23
    input[3] -> c3 => q30, q31, q32, q33

    q00, q10, q20, q30 => circuit_link0 -> [0]output
    q01, q11, q21, q31 => circuit_link1 -> [1]output
    q02, q12, q22, q32 => circuit_link2 -> [2]output
    q03, q13, q23, q33 => circuit_link3 -> [3]output

    q00, q01, q02, q03 => packet_up_link0 -> [0]ps[0] -> [0]output
    q10, q11, q12, q13 => packet_up_link1 -> [1]ps[1] -> [1]output
    q20, q21, q22, q23 => packet_up_link2 -> [2]ps[2] -> [2]output
    q30, q31, q32, q33 => packet_up_link3 -> [3]ps[3] -> [3]output
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16) 
   -> pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -)[1]
   -> in_classfy[0,1,2,3]
   => hybrid_switch[0,1,2,3]
   -> arp -> out

arp_c[1] -> [1]arp
arp_c[2] -> arp_r -> out

pc -> ICMPPingResponder -> arp
