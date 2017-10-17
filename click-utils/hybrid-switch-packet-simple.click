define($DEVNAME enp8s0)

define($IP0 10.10.1.8, $IP1 10.10.1.9)

define ($PACKET_BW 0.5Gbps)
define ($DELAY_LATENCY 0.1)

StaticThreadSched(in 1,)
		  hybrid_switch/packet_up_link0 6,
                  hybrid_switch/packet_up_link1 6,
		  hybrid_switch/ps/packet_link0 6,
		  hybrid_switch/ps/packet_link1 6,
		  )

t :: ARPTable
elementclass dp { $num |
in :: FromDPDKDevice(0, BURST 1)
out :: ToDPDKDevice(0)

arp_c :: Classifier(12/0800, 12/0806 20/0002, 12/0806 20/0001)
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth, TABLE t)
arp_r :: ARPResponder($DEVNAME)

elementclass in_classfy {
    input[0] -> IPClassifier(src host $IP0, src host $IP1)
             => [0, 1]output
}

elementclass out_classfy {
    input[0] -> IPClassifier(dst host $IP0, dst host $IP1)
             => [0, 1]output
}

elementclass packet_link {
    input[0, 1]
                           => RoundRobinSched 
			   -> LinkUnqueue($DELAY_LATENCY, $PACKET_BW)
			   -> output
}

elementclass packet_switch {
    c0, c1 :: out_classfy

    q00, q01, q10, q11 :: Queue(CAPACITY 1)

    packet_link0, packet_link1 :: packet_link

    input[0] -> c0 => q00, q01
    input[1] -> c1 => q10, q11

    q00, q10 => packet_link0 -> [0]output
    q01, q11 => packet_link1 -> [1]output

    // would be drops
    q00[1], q01[1] -> [2]output
    q10[1], q11[1] -> [3]output
}

hybrid_switch :: {
    c0, c1 :: out_classfy

    // ToR queues (in here for convenience)
    q00, q01, q10, q11
 :: {
        input[0] -> q ::Queue(CAPACITY 100)
	input[1] -> lq :: Queue(CAPACITY 1) // loss queue
	lq, q => PrioSched -> output
    }

    packet_up_link0, packet_up_link1 :: packet_link

    ps :: packet_switch

    input[0] -> c0 => q00, q01
    input[1] -> c1 => q10, q11

    q00, q01 => packet_up_link0 -> [0]ps[0] -> [0]output
    q10, q11 => packet_up_link1 -> [1]ps[1] -> [1]output

    // dropped PS packets -> loss queues
    ps[2] -> out_classfy => [1]q00, [1]q01
    ps[3] -> out_classfy => [1]q10, [1]q11
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16) 
   -> pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -)[1]
   -> in_classfy[0,1]
   => hybrid_switch[0,1]
   -> arp -> out

arp_c[1] -> Print -> [1]arp
arp_c[2] -> arp_r -> out

pc -> ICMPPingResponder -> arp
