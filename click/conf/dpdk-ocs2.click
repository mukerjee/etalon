define($IP0 1.1.1.1, $IP1 1.1.1.2, $IP2 1.1.1.3, $IP3 1.1.1.4,
       $MAC0 0A:02:03:04:05:06, $MAC1 0B:02:03:04:05:06,
       $MAC2 0C:02:03:04:05:06, $MAC3 0D:02:03:04:05:06,
       $MACSwitch 0F:02:03:04:05:06)

define ($CIRCUIT_BW 10Gbps, $PACKET_BW 1Gbps)

define ($RECONFIG_DELAY 0.000003)
// define ($RECONFIG_DELAY 0.3)

ControlSocket("TCP", 1239)

// Script(write hybrid_switch/s01.switch 1,
//        write hybrid_switch/s12.switch 1,
//        write hybrid_switch/s23.switch 1,
//        write hybrid_switch/s30.switch 1)

Script(write hybrid_switch/s01.switch 1,
       write hybrid_switch/s12.switch 1,
       write hybrid_switch/s23.switch 1,
       write hybrid_switch/s30.switch 1,
       wait $(mul 9 $RECONFIG_DELAY),
       write hybrid_switch/s01.switch 0,
       write hybrid_switch/s12.switch 0,
       write hybrid_switch/s23.switch 0,
       write hybrid_switch/s30.switch 0,
       wait $RECONFIG_DELAY,
       write hybrid_switch/s02.switch 1,
       write hybrid_switch/s13.switch 1,
       write hybrid_switch/s20.switch 1,
       write hybrid_switch/s31.switch 1,
       wait $(mul 9 $RECONFIG_DELAY),
       write hybrid_switch/s02.switch 0,
       write hybrid_switch/s13.switch 0,
       write hybrid_switch/s20.switch 0,
       write hybrid_switch/s31.switch 0,
       wait $RECONFIG_DELAY,
       write hybrid_switch/s03.switch 1,
       write hybrid_switch/s10.switch 1,
       write hybrid_switch/s21.switch 1,
       write hybrid_switch/s32.switch 1,
       wait $(mul 9 $RECONFIG_DELAY),
       write hybrid_switch/s03.switch 0,
       write hybrid_switch/s10.switch 0,
       write hybrid_switch/s21.switch 0,
       write hybrid_switch/s32.switch 0,
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


in :: FromDPDKDevice(0)
out :: ToDPDKDevice(0)
// in :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP0, $IP1)
//        -> EtherEncap(0x0800, $MAC0, $MAC1) -> output}
// out :: Discard()

elementclass Checker { $mac |
    c :: IPClassifier(src host $IP0, src host $IP1, src host $IP2, src host $IP3)
    c0, c1, c2, c3 :: Counter
    input -> c => c0, c1, c2, c3
          -> StoreData(0, DATA \<$mac>) -> StoreData(6, DATA \<$MACSwitch>)
          -> output
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

    s00, s01, s02, s03 :: Switch(0)
    s10, s11, s12, s13 :: Switch(0)
    s20, s21, s22, s23 :: Switch(0)
    s30, s31, s32, s33 :: Switch(0)

    packet_link0, packet_link1, packet_link2, packet_link3 ::
      {input -> Queue(CAPACITY 1000) -> LinkUnqueue(0, $PACKET_BW) -> output}

    circuit_link0, circuit_link1, circuit_link2, circuit_link3 ::
      {input -> Queue(CAPACITY 1000) -> LinkUnqueue(0, $CIRCUIT_BW) -> output}

    input[0] -> c0 => s00, s01, s02, s03
    input[1] -> c1 => s10, s11, s12, s13
    input[2] -> c2 => s20, s21, s22, s23
    input[3] -> c3 => s30, s31, s32, s33

    s00, s10, s20, s30 -> packet_link0
    s01, s11, s21, s31 -> packet_link1
    s02, s12, s22, s32 -> packet_link2
    s03, s13, s23, s33 -> packet_link3

    s00[1], s10[1], s20[1], s30[1] -> circuit_link0
    s01[1], s11[1], s21[1], s31[1] -> circuit_link1
    s02[1], s12[1], s22[1], s32[1] -> circuit_link2
    s03[1], s13[1], s23[1], s33[1] -> circuit_link3

    packet_link0, circuit_link0 -> [0]output
    packet_link1, circuit_link1 -> [1]output
    packet_link2, circuit_link2 -> [2]output
    packet_link3, circuit_link3 -> [3]output
}

in -> MarkIPHeader(14)
   -> IPClassifier(src host $IP0, src host $IP1,
                   src host $IP2, src host $IP3)[0, 1, 2, 3]
   => hybrid_switch => out0, out1, out2, out3 -> out
