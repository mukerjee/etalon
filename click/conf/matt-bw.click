define($IP0 1.1.1.1, $IP1 1.1.1.2, $IP2 1.1.1.3, $IP3 1.1.1.4,
       $MAC0 0A:02:03:04:05:06, $MAC1 0B:02:03:04:05:06,
       $MACSwitch 0F:02:03:04:05:06)

define ($PACKET_BW 10Gbps)

cs :: ControlSocket("TCP", 1239)

elementclass Checker { $mac |
    c :: IPClassifier(src host $IP0, src host $IP1, src host $IP2, src host $IP3)
    c0, c1, c2, c3 :: Counter
    input -> c => c0, c1, c2, c3
          -> StoreData(0, DATA \<$mac>) -> StoreData(6, DATA \<$MACSwitch>)
          -> output
}

out1 :: Checker($MAC1)

InfiniteSource(LENGTH 1500)
   -> IPEncap(255, $IP0, $IP1)
   -> EtherEncap(0x0800, $MAC0, $MAC1)
   // -> Queue(CAPACITY 1000)
   // -> RoundRobinSched
   -> LinkUnqueue(0, $PACKET_BW)
   -> out1
   -> ToDump("./test.pcap", SNAPLEN 34)
   -> Discard