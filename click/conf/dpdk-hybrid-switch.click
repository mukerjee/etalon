define($DEVNAME enp8s0d1)

define($NUMHOSTS 4)
define($IP0 10.10.1.2, $IP1 10.10.1.3, $IP2 10.10.1.4, $IP3 10.10.1.5)

define ($CIRCUIT_BW 1Gbps, $PACKET_BW 0.1Gbps)

define ($RECONFIG_DELAY 20) // usecs
define ($TDF 1)

StaticThreadSched(hybrid_switch/packet_link0 1,
                  hybrid_switch/packet_link1 2,
                  hybrid_switch/packet_link2 3,
                  hybrid_switch/packet_link3 4,
                  hybrid_switch/circuit_link0 1,
                  hybrid_switch/circuit_link1 2,
                  hybrid_switch/circuit_link2 3,
                  hybrid_switch/circuit_link3 4,
		  runner 5,
		  sol 6,
		  )

ControlSocket("TCP", 1239)

sol :: Solstice($NUMHOSTS, $CIRCUIT_BW, $PACKET_BW, $RECONFIG_DELAY, $TDF)
runner :: RunSchedule($NUMHOSTS)

in :: FromDPDKDevice(1)
out :: ToDPDKDevice(1)

arp_c :: Classifier(12/0800, 12/0806 20/0002)
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth)

hybrid_switch :: {
    c0 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c1 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c2 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c3 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)

    q00, q01, q02, q03 :: Queue(CAPACITY 1000)
    q10, q11, q12, q13 :: Queue(CAPACITY 1000)
    q20, q21, q22, q23 :: Queue(CAPACITY 1000)
    q30, q31, q32, q33 :: Queue(CAPACITY 1000)

    packet_up_link0, packet_up_link1, packet_up_link2, packet_up_link3 :: {
      input[0,1,2,3] => RoundRobinSched -> LinkUnqueue(0, $PACKET_BW)
		     -> output
    }

    circuit_link0, circuit_link1, circuit_link2, circuit_link3 :: {
      input[0,1,2,3] => ps :: PullSwitch(-1) -> LinkUnqueue(0, $CIRCUIT_BW)
		     -> output
    }

    input[0] -> c0 => q00, q01, q02, q03
    input[1] -> c1 => q10, q11, q12, q13
    input[2] -> c2 => q20, q21, q22, q23
    input[3] -> c3 => q30, q31, q32, q33

    // q00, q10, q20, q30 => packet_link0 -> [0]output
    // q01, q11, q21, q31 => packet_link1 -> [1]output
    // q02, q12, q22, q32 => packet_link2 -> [2]output
    // q03, q13, q23, q33 => packet_link3 -> [3]output

    packet_down_link0, packet_down_link1, packet_down_link2, packet_down_link3 :: BandwidthRatedSplitter($PACKET_BW)
    q00, q01, q02, q03 -> packet_up_link0 -> IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3) => packet_down_link0, packet_down_link1, packet_down_link2, packet_down_link3
    q10, q11, q12, q13 -> packet_up_link1 -> IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3) => packet_down_link0, packet_down_link1, packet_down_link2, packet_down_link3
    q20, q21, q22, q23 -> packet_up_link2 -> IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3) => packet_down_link0, packet_down_link1, packet_down_link2, packet_down_link3
    q30, q31, q32, q33 -> packet_up_link3 -> IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3) => packet_down_link0, packet_down_link1, packet_down_link2, packet_down_link3
    packet_down_link0 => [0]output, Discard
    packet_down_link1 => [1]output, Discard
    packet_down_link2 => [2]output, Discard
    packet_down_link3 => [3]output, Discard

    q00, q10, q20, q30 => circuit_link0 -> [0]output
    q01, q11, q21, q31 => circuit_link1 -> [1]output
    q02, q12, q22, q32 => circuit_link2 -> [2]output
    q03, q13, q23, q33 => circuit_link3 -> [3]output
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16)
   -> IPClassifier(src host $IP0, src host $IP1,
                   src host $IP2, src host $IP3)[0,1,2,3]
   => hybrid_switch[0,1,2,3]
   -> arp -> out

arp_c[1] -> [1]arp
