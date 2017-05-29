define($IP0 10.10.1.2, $IP1 10.10.1.3, $IP2 10.10.1.4, $IP3 10.10.1.5,
       $IPSwitch 10.10.1.1,
       $MAC0 f4:52:14:15:6f:d2, $MAC1 f4:52:14:15:6d:32,
       $MAC2 0C:02:03:04:05:06, $MAC3 0D:02:03:04:05:06,
       $MACSwitch f4:52:14:15:6c:02)

define ($CIRCUIT_BW 10Gbps, $PACKET_BW 1Gbps)

define ($RECONFIG_DELAY 0.000020)

StaticThreadSched(scripte 7,
                  hybrid_switch/c0 0,
                  hybrid_switch/c1 1,
                  hybrid_switch/c2 2,
                  hybrid_switch/c3 3,
                  hybrid_switch/q00 0,
                  hybrid_switch/q01 0,
                  hybrid_switch/q02 0,
                  hybrid_switch/q03 0,
                  hybrid_switch/q10 1,
                  hybrid_switch/q11 1,
                  hybrid_switch/q12 1,
                  hybrid_switch/q13 1,
                  hybrid_switch/q20 2,
                  hybrid_switch/q21 2,
                  hybrid_switch/q22 2,
                  hybrid_switch/q23 2,
                  hybrid_switch/q30 3,
                  hybrid_switch/q31 3,
                  hybrid_switch/q32 3,
                  hybrid_switch/q33 3,
                  hybrid_switch/packet_link0 0,
                  hybrid_switch/packet_link1 1,
                  hybrid_switch/packet_link2 2,
                  hybrid_switch/packet_link3 3,
                  hybrid_switch/circuit_link0 0,
                  hybrid_switch/circuit_link1 1,
                  hybrid_switch/circuit_link2 2,
                  hybrid_switch/circuit_link3 3,
		  hybrid_switch/rrs0 0,
		  hybrid_switch/rrs1 1,
		  hybrid_switch/rrs2 2,
		  hybrid_switch/rrs3 3,
		  hybrid_switch/ps0 0,
		  hybrid_switch/ps1 1,
		  hybrid_switch/ps2 2,
		  hybrid_switch/ps3 3,
                  hybrid_switch/ecnr0 0,
                  hybrid_switch/ecnr1 1,
                  hybrid_switch/ecnr2 2,
                  hybrid_switch/ecnr3 3,
                  out0 0,
                  out1 1,
                  out2 2,
                  out3 3,
                  in 4,
                  miph 4,
                  ipc 4,
                  out 5,
                  cs 6,
		  // arp_c 4,
		  // arp 5
		  )

cs :: ControlSocket("TCP", 1239)

scripte :: Script_New(
       write hybrid_switch/ps0.switch 3,
       write hybrid_switch/ps1.switch 0,
       write hybrid_switch/ps2.switch 1,
       write hybrid_switch/ps3.switch 2,
       write hybrid_switch/ecnr0/s.switch 1,
       write hybrid_switch/ecnr1/s.switch 2,
       write hybrid_switch/ecnr2/s.switch 3,
       write hybrid_switch/ecnr3/s.switch 0,
       wait $(mul 9 $RECONFIG_DELAY),
       write hybrid_switch/ps0.switch -1,
       write hybrid_switch/ps1.switch -1,
       write hybrid_switch/ps2.switch -1,
       write hybrid_switch/ps3.switch -1,
       write hybrid_switch/ecnr0/s.switch 4,
       write hybrid_switch/ecnr1/s.switch 4,
       write hybrid_switch/ecnr2/s.switch 4,
       write hybrid_switch/ecnr3/s.switch 4,
       wait $RECONFIG_DELAY,
       write hybrid_switch/ps0.switch 2,
       write hybrid_switch/ps1.switch 3,
       write hybrid_switch/ps2.switch 0,
       write hybrid_switch/ps3.switch 1,
       write hybrid_switch/ecnr0/s.switch 2,
       write hybrid_switch/ecnr1/s.switch 3,
       write hybrid_switch/ecnr2/s.switch 0,
       write hybrid_switch/ecnr3/s.switch 1,
       wait $(mul 9 $RECONFIG_DELAY),
       write hybrid_switch/ps0.switch -1,
       write hybrid_switch/ps1.switch -1,
       write hybrid_switch/ps2.switch -1,
       write hybrid_switch/ps3.switch -1,
       write hybrid_switch/ecnr0/s.switch 4,
       write hybrid_switch/ecnr1/s.switch 4,
       write hybrid_switch/ecnr2/s.switch 4,
       write hybrid_switch/ecnr3/s.switch 4,
       wait $RECONFIG_DELAY,
       write hybrid_switch/ps0.switch 1,
       write hybrid_switch/ps1.switch 2,
       write hybrid_switch/ps2.switch 3,
       write hybrid_switch/ps3.switch 0,
       write hybrid_switch/ecnr0/s.switch 3,
       write hybrid_switch/ecnr1/s.switch 0,
       write hybrid_switch/ecnr2/s.switch 1,
       write hybrid_switch/ecnr3/s.switch 2,
       wait $(mul 9 $RECONFIG_DELAY),
       write hybrid_switch/ps0.switch -1,
       write hybrid_switch/ps1.switch -1,
       write hybrid_switch/ps2.switch -1,
       write hybrid_switch/ps3.switch -1,
       write hybrid_switch/ecnr0/s.switch 4,
       write hybrid_switch/ecnr1/s.switch 4,
       write hybrid_switch/ecnr2/s.switch 4,
       write hybrid_switch/ecnr3/s.switch 4,
       wait $RECONFIG_DELAY,
       loop)

// 0 1 0 0
// 0 0 1 0
// 0 0 0 1
// 1 0 0 0

// 0 0 1 0
// 0 0 0 1
// 1 0 0 0
// 0 1 0 0

// 0 0 0 1
// 1 0 0 0
// 0 1 0 0
// 0 0 1 0


// in :: FromDPDKDevice(0)
// out :: ToDPDKDevice(0)
in :: {InfiniteSource(LENGTH 1500) -> IPEncap(255, $IP0, $IP1)
       -> EtherEncap(0x0800, $MAC0, $MAC1) -> output}
out :: ToDump("./test.pcap", SNAPLEN 34)

// arp_c :: Classifier(12/0806 20/0002, 12/0800);
// arp :: ARPQuerier($IPSwitch, $MACSwitch)


elementclass Checker { $mac |
    c :: IPClassifier(src host $IP0, src host $IP1, src host $IP2, src host $IP3)
    c0, c1, c2, c3 :: Counter
    input -> c => c0, c1, c2, c3
          -> StoreData(0, DATA \<$mac>) -> StoreData(6, DATA \<$MACSwitch>)
          -> output
}

elementclass ECNRewriter {
    s :: Switch(4)
    w0 :: StoreData(15, DATA \<00>)
    w1 :: StoreData(15, DATA \<01>)
    w2 :: StoreData(15, DATA \<02>)
    w3 :: StoreData(15, DATA \<03>)
    wNone :: StoreData(15, DATA \<FC>)
    input -> s => w0, w1, w2, w3, wNone -> output
}

out0 :: Checker($MAC0)
out1 :: Checker($MAC1)
out2 :: Checker($MAC2)
out3 :: Checker($MAC3)

hybrid_switch :: {
    c0 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c1 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c2 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c3 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)

    q00, q01, q02, q03 :: ThreadSafeQueue(CAPACITY 1000)
    q10, q11, q12, q13 :: ThreadSafeQueue(CAPACITY 1000)
    q20, q21, q22, q23 :: ThreadSafeQueue(CAPACITY 1000)
    q30, q31, q32, q33 :: ThreadSafeQueue(CAPACITY 1000)

    packet_link0, packet_link1, packet_link2, packet_link3 ::
      {input -> LinkUnqueue(0, $PACKET_BW) ->
      StoreData(18, DATA \<01>) -> output}

    circuit_link0, circuit_link1, circuit_link2, circuit_link3 ::
      {input -> LinkUnqueue(0, $CIRCUIT_BW) ->
      StoreData(18, DATA \<02>) -> output}

    input[0] -> c0 => q00, q01, q02, q03
    input[1] -> c1 => q10, q11, q12, q13
    input[2] -> c2 => q20, q21, q22, q23
    input[3] -> c3 => q30, q31, q32, q33

    q00, q10, q20, q30 => rrs0 :: RoundRobinSched -> packet_link0
    q01, q11, q21, q31 => rrs1 :: RoundRobinSched -> packet_link1
    q02, q12, q22, q32 => rrs2 :: RoundRobinSched -> packet_link2
    q03, q13, q23, q33 => rrs3 :: RoundRobinSched -> packet_link3

    q00, q10, q20, q30 => ps0 :: PullSwitch(0) -> circuit_link0
    q01, q11, q21, q31 => ps1 :: PullSwitch(1) -> circuit_link1
    q02, q12, q22, q32 => ps2 :: PullSwitch(2) -> circuit_link2
    q03, q13, q23, q33 => ps3 :: PullSwitch(3) -> circuit_link3

    packet_link0, circuit_link0 -> ecnr0 :: ECNRewriter -> [0]output
    packet_link1, circuit_link1 -> ecnr1 :: ECNRewriter -> [1]output
    packet_link2, circuit_link2 -> ecnr2 :: ECNRewriter -> [2]output
    packet_link3, circuit_link3 -> ecnr3 :: ECNRewriter -> [3]output
}

// arp_c[0] -> [1]arp

in -> //arp_c[1] ->
miph :: MarkIPHeader(14)
   -> ipc ::IPClassifier(src host $IP0, src host $IP1,
                         src host $IP2, src host $IP3)[0, 1, 2, 3]
   => hybrid_switch => out0, out1, out2, out3 -> //arp ->
   out
