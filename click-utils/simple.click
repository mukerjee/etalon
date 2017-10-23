define($DEVNAME enp8s0)

define($IP1 10.10.1.1, $IP2 10.10.1.2)

define ($PACKET_BW 0.5Gbps)
define ($DELAY_LATENCY 0.1)

StaticThreadSched(in 1,
                  l1 6,
                  // l2 6,
                  )

in :: FromDPDKDevice(0)
out :: ToDPDKDevice(0)

arp_c :: Classifier(12/0800, 12/0806 20/0002, 12/0806 20/0001)
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth)
arp_r :: ARPResponder($DEVNAME)

elementclass in_classfy {
    input[0] -> IPClassifier(src host $IP1, src host $IP2)
             => [0, 1]output
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16)
   -> pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -)[1]
   -> ic :: in_classfy

   // ic[0] -> Queue(CAPACITY 10000) -> LinkUnqueue($DELAY_LATENCY, $PACKET_BW) -> arp
   // ic[1] -> Queue(CAPACITY 10000) -> LinkUnqueue($DELAY_LATENCY, $PACKET_BW) -> arp
   ic[0,1] -> q1 :: Queue(CAPACITY 10000) -> l1 :: LinkUnqueue(0, 0.5Gbps) -> arp
   // ic[1] -> Queue(CAPACITY 100) -> LinkUnqueue(0, 0.5Gbps) -> arp
   arp -> out

arp_c[1] -> [1]arp
arp_c[2] -> arp_r -> out

Script(wait 1, print q1.length, loop)

pc -> ICMPPingResponder -> arp
