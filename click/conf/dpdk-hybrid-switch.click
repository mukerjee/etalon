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

define($IP0 10.10.1.2, $IP1 10.10.1.3, $IP2 10.10.1.4, $IP3 10.10.1.5)

define ($CIRCUIT_BW 8Gbps, $PACKET_BW 1Gbps)
define ($RTT 60)  // usecs -- measured
define ($MTU 9000)  // bytes
define ($DELAY_LATENCY 0.000140) // seconds 10 - (RTT / TDF) * TDF, if your target is 10us

// TODO emperical
define ($CQL 170)  // ($CIRCUIT_BW / 8.0 / $MTU) * $RTT

define ($RECONFIG_DELAY 20)  // usecs
define ($TDF 20)

StaticThreadSched(in 0,
		  runner 1,
		  sol 2,
 		  hybrid_switch/packet_up_link0 3,
                  hybrid_switch/packet_up_link1 3,
                  hybrid_switch/packet_up_link2 3,
                  hybrid_switch/packet_up_link3 3,
		  hybrid_switch/ps/packet_link0 3,
		  hybrid_switch/ps/packet_link1 3,
		  hybrid_switch/ps/packet_link2 3,
		  hybrid_switch/ps/packet_link3 3,
                  hybrid_switch/circuit_link0 4,
                  hybrid_switch/circuit_link1 5,
                  hybrid_switch/circuit_link2 6,
                  hybrid_switch/circuit_link3 7,
		  )

ControlSocket("TCP", 1239)

sol :: Solstice($NUMHOSTS, $CIRCUIT_BW, $PACKET_BW, $RECONFIG_DELAY, $TDF)
runner :: RunSchedule($NUMHOSTS)
// Script(write hybrid_switch/circuit_link0/ps.switch 1,
//        write hybrid_switch/circuit_link1/ps.switch 0,
//        write hybrid_switch/circuit_link2/ps.switch 3,
//        write hybrid_switch/circuit_link3/ps.switch 2)

in :: FromDPDKDevice(0)
out :: ToDPDKDevice(0)

arp_c :: Classifier(12/0800, 12/0806 20/0002, 12/0806 20/0001)
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth)
arp_r :: ARPResponder($DEVNAME)

elementclass classfy {
    input[0] -> IPClassifier(dst host $IP0, dst host $IP1, 
                             dst host $IP2, dst host $IP3)
             => [0, 1, 2, 3]output
}

elementclass packet_link {
    input[0,1,2,3] 
                   => RoundRobinSched 
                   -> LinkUnqueue($DELAY_LATENCY, $PACKET_BW)
		   -> output
}

elementclass circuit_link {
    input[0,1,2,3] => ps :: PullSwitch(-1) -> LinkUnqueue($DELAY_LATENCY, $CIRCUIT_BW)
		   -> output
}

elementclass packet_switch {
    c0, c1, c2, c3 :: classfy
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

    q00[1], q01[1], q02[1], q03[1] => [ 4,  5,  6,  7]output
    q10[1], q11[1], q12[1], q13[1] => [ 8,  9, 10, 11]output
    q20[1], q21[1], q22[1], q23[1] => [12, 13, 14, 15]output
    q30[1], q31[1], q32[1], q33[1] => [16, 17, 18, 19]output
}

hybrid_switch :: {
    c0, c1, c2, c3 :: classfy

    // ToR queues (in here for convenience)
    q00, q01, q02, q03,
    q10, q11, q12, q13,
    q20, q21, q22, q23,
    q30, q31, q32, q33 :: {
        input[0] -> q ::Queue(CAPACITY $CQL)
	input[1] -> lq :: Queue(CAPACITY 1) // loss queue
	lq, q => PrioSched -> output
	// q -> output
    }

    circuit_link0, circuit_link1, circuit_link2, circuit_link3 :: circuit_link
    packet_up_link0, packet_up_link1, packet_up_link2, packet_up_link3 :: packet_link
    ps :: packet_switch

    input[0] -> c0 => [0]q00, [0]q01, [0]q02, [0]q03
    input[1] -> c1 => [0]q10, [0]q11, [0]q12, [0]q13
    input[2] -> c2 => [0]q20, [0]q21, [0]q22, [0]q23
    input[3] -> c3 => [0]q30, [0]q31, [0]q32, [0]q33

    q00, q10, q20, q30 => circuit_link0 -> [0]output
    q01, q11, q21, q31 => circuit_link1 -> [1]output
    q02, q12, q22, q32 => circuit_link2 -> [2]output
    q03, q13, q23, q33 => circuit_link3 -> [3]output

    q00, q01, q02, q03 => packet_up_link0 -> [0]ps[0] -> [0]output
    q10, q11, q12, q13 => packet_up_link1 -> [1]ps[1] -> [1]output
    q20, q21, q22, q23 => packet_up_link2 -> [2]ps[2] -> [2]output
    q30, q31, q32, q33 => packet_up_link3 -> [3]ps[3] -> [3]output

    ps[ 4,  5,  6,  7] => [1]q00, [1]q01, [1]q02, [1]q03
    ps[ 8,  9, 10, 11] => [1]q10, [1]q11, [1]q12, [1]q13
    ps[12, 13, 14, 15] => [1]q20, [1]q21, [1]q22, [1]q23
    ps[16, 17, 18, 19] => [1]q30, [1]q31, [1]q32, [1]q33
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16) 
   -> pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -)[1]
   -> IPClassifier(src host $IP0, src host $IP1,
                   src host $IP2, src host $IP3)[0,1,2,3]
   => hybrid_switch[0,1,2,3]
   -> arp -> out

arp_c[1] -> [1]arp
arp_c[2] -> arp_r -> out

pc -> ICMPPingResponder -> Queue -> DelayUnqueue($DELAY_LATENCY) -> arp 
