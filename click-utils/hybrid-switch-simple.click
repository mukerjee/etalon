define($DEVNAME enp8s0)

define($NUMHOSTS 2)
define($IP11 10.10.1.11, $IP12 10.10.1.12, $IP13 10.10.1.13, $IP14 10.10.1.14,
       $IP15 10.10.1.15, $IP16 10.10.1.16, $IP17 10.10.1.17, $IP18 10.10.1.18,
       $IP21 10.10.1.21, $IP22 10.10.1.22, $IP23 10.10.1.23, $IP24 10.10.1.24,
       $IP25 10.10.1.25, $IP26 10.10.1.26, $IP27 10.10.1.27, $IP28 10.10.1.28)

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
		  hybrid_switch/packet_up_link0 6,
                  hybrid_switch/packet_up_link1 6,
		  hybrid_switch/ps/packet_link0 6,
		  hybrid_switch/ps/packet_link1 6,
		  )

ControlSocket("TCP", 1239)

traffic_matrix :: EstimateTraffic($NUMHOSTS, SOURCE QUEUE)
sol :: Solstice($NUMHOSTS, $CIRCUIT_BW, $PACKET_BW, $RECONFIG_DELAY, $TDF)
runner :: RunSchedule($NUMHOSTS, $BIG_BUFFER_SIZE, $SMALL_BUFFER_SIZE, RESIZE false)

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
)
             => [0, 1]output
}

elementclass out_classfy {
    input[0] -> IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
)
             => [0, 1]output
}

elementclass packet_link {
    input[0,1] 
                           => RoundRobinSched 
                  	   -> BandwidthRatedUnqueue($PACKET_BW)
			   -> output
}

elementclass circuit_link {
    input[0,1] 
                           => ps :: PullSwitch(-1)
                           -> BandwidthRatedUnqueue($CIRCUIT_BW)
		           -> output
}

elementclass packet_switch {
    c0, c1  :: out_classfy

    q00, q01, q10, q11 :: Queue(CAPACITY 1)

    packet_link0, packet_link1 :: packet_link

    input[0] -> c0 => q00, q01
    input[1] -> c1 => q10, q11

    q00, q10 => packet_link0 -> [0]output
    q01, q11 => packet_link1 -> [1]output
}

hybrid_switch :: {
    c0, c1 :: out_classfy

    // ToR queues (in here for convenience)
    q00, q01, q10, q11 :: Queue(CAPACITY 100)

    packet_up_link0, packet_up_link1 :: packet_link
    ps :: packet_switch

    circuit_link0, circuit_link1 :: circuit_link

    input[0] -> c0 => q00, q01
    input[1] -> c1 => q10, q11

    q00, q10 => circuit_link0 -> [0]output
    q01, q11 => circuit_link1 -> [1]output

    q00, q01 => packet_up_link0 -> [0]ps[0] -> [0]output
    q10, q11 => packet_up_link1 -> [1]ps[1] -> [1]output
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16) 
   -> pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -)[1]
   -> in_classfy[0,1]
   => hybrid_switch[0,1]
   -> arp -> out

arp_c[1] -> [1]arp
arp_c[2] -> arp_r -> out

pc -> ICMPPingResponder -> arp 
