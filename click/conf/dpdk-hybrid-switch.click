define($DEVNAME eth2)

define($NUMHOSTS 4)
define($IP0 10.10.1.2, $IP1 10.10.1.3, $IP2 10.10.1.4, $IP3 10.10.1.5)

define ($CIRCUIT_BW 10Gbps, $PACKET_BW 1Gbps)
define ($RTT 200)  // usecs
define ($MTU 1500)  // bytes

// TODO emperical
define ($CQL 170, $PQL 20)  // ($CIRCUIT_BW / 8.0 / $MTU) * $RTT

define ($RECONFIG_DELAY 20)  // usecs
define ($TDF 1)

StaticThreadSched(hybrid_switch/packet_up_link0 1,
                  hybrid_switch/packet_up_link1 2,
                  hybrid_switch/packet_up_link2 3,
                  hybrid_switch/packet_up_link3 4,
		  hybrid_switch/ps/packet_link0 1,
		  hybrid_switch/ps/packet_link1 2,
		  hybrid_switch/ps/packet_link2 3,
		  hybrid_switch/ps/packet_link3 4,
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

in :: FromDPDKDevice(0)
out :: ToDPDKDevice(0)

arp_c :: Classifier(12/0800, 12/0806 20/0002)
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth)

elementclass classfy {
    input[0] -> IPClassifier(dst host $IP0, dst host $IP1, 
                             dst host $IP2, dst host $IP3)
             => [0, 1, 2, 3]output
}

elementclass packet_link {
    input[0,1,2,3] 
                   => RoundRobinSched 
		   // => dps :: DontPullSwitch(-1)
                   -> LinkUnqueue(0, $PACKET_BW)
		   -> output
}

elementclass circuit_link {
    input[0,1,2,3] => ps :: PullSwitch(-1) -> LinkUnqueue(0, $CIRCUIT_BW)
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

    // ps[ 4,  5,  6,  7] => Discard, Discard, Discard, Discard
    // ps[ 8,  9, 10, 11] => Discard, Discard, Discard, Discard
    // ps[12, 13, 14, 15] => Discard, Discard, Discard, Discard
    // ps[16, 17, 18, 19] => Discard, Discard, Discard, Discard

    ps[ 4,  5,  6,  7] => [1]q00, [1]q01, [1]q02, [1]q03
    ps[ 8,  9, 10, 11] => [1]q10, [1]q11, [1]q12, [1]q13
    ps[12, 13, 14, 15] => [1]q20, [1]q21, [1]q22, [1]q23
    ps[16, 17, 18, 19] => [1]q30, [1]q31, [1]q32, [1]q33
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16)
   -> IPClassifier(src host $IP0, src host $IP1,
                   src host $IP2, src host $IP3)[0,1,2,3]
   => hybrid_switch[0,1,2,3]
   -> arp -> out

arp_c[1] -> [1]arp
