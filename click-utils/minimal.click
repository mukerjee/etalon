define($DEVNAME enp8s0)
define($NUM_RACKS 2)

define($IP1 10.10.1.1, $IP2 10.10.1.2)

define ($CIRCUIT_BW 4Gbps, $PACKET_BW 0.5Gbps)
define ($PACKET_LATENCY 0.000100)
define ($CIRCUIT_LATENCY 0.000200)
define ($BIG_QUEUE_SIZE 64)

Script(write runner.setSchedule 14 3600 1/-1 400 -1/-1 3600 -1/-1 400 -1/-1 3600 -1/-1 400 -1/-1 3600 -1/-1 400 -1/-1 3600 -1/-1 400 -1/-1 3600 -1/-1 400 -1/-1 3600 -1/0 400 -1/-1)
// Script(write runner.setSchedule 14 36000 1/-1 4000 -1/-1 36000 -1/-1 4000 -1/-1 36000 -1/-1 4000 -1/-1 36000 -1/-1 4000 -1/-1 36000 -1/-1 4000 -1/-1 36000 -1/-1 4000 -1/-1 36000 -1/0 4000 -1/-1)
// Script(write runner.setSchedule 1 3600 1/0)
// Script(write runner.setSchedule 1 3600 -1/-1)
// Script(write runner.setDoResize true)


StaticThreadSched(in 0,
                  runner 3,
                  hybrid_switch/circuit_link1 4,
                  hybrid_switch/circuit_link2 4,
                  hybrid_switch/packet_up_link1 6,
                  hybrid_switch/packet_up_link2 6,
                  hybrid_switch/ps/packet_link1 6,
                  hybrid_switch/ps/packet_link2 6,
)

ControlSocket("TCP", 1239)

runner :: RunSchedule($NUM_RACKS, RESIZE false)

in :: FromDPDKDevice(0)
out :: ToDPDKDevice(0)

arp_c :: Classifier(12/0800, 12/0806 20/0002, 12/0806 20/0001)
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth)
arp_r :: ARPResponder($DEVNAME)

elementclass in_classfy {
  input[0] -> IPClassifier(
    src host $IP1,
    src host $IP2,
  )
  => [0, 1]output
}

elementclass out_classfy {
  input[0] -> IPClassifier(
    dst host $IP1,
    dst host $IP2,
  )
  => [0, 1]output
}

elementclass packet_link {
  input[0, 1]
    => RoundRobinSched
    -> LinkUnqueue($PACKET_LATENCY, $PACKET_BW)
    -> output
}

elementclass circuit_link {
  input[0, 1]
    => ps :: SimplePullSwitch(-1)
    -> LinkUnqueue($CIRCUIT_LATENCY, $CIRCUIT_BW)
    -> StoreData(1, 1)
    -> output
}

elementclass packet_switch {
  c1, c2 :: out_classfy

  q11, q12, q21, q22 :: Queue(CAPACITY 3)

  packet_link1, packet_link2 :: packet_link

  input[0] -> c1 => q11, q12
  input[1] -> c2 => q21, q22

  q11, q21  => packet_link1 -> [0]output
  q12, q22  => packet_link2 -> [1]output

  q11[1], q12[1] -> [2]output
  q21[1], q22[1] -> [3]output
}

hybrid_switch :: {
  c1, c2 :: out_classfy

  q11, q12, q21, q22 :: {
      input[0] -> q :: Queue(CAPACITY $BIG_QUEUE_SIZE)
      input[1] -> lq :: Queue(CAPACITY 3)  // loss queue
      lq, q => PrioSched -> output
      lq[1] -> Print("LQ DROP") -> Discard
 }

  circuit_link1, circuit_link2 :: circuit_link

  packet_up_link1, packet_up_link2 :: packet_link

  ps :: packet_switch

  input[0] -> Paint(1, 20) -> c1 => q11, q12
  input[1] -> Paint(2, 20) -> c2 => q21, q22

  q11, q21 => circuit_link1 -> coc1 :: Paint(0, 23) -> Paint(1, 22) -> Paint(1, 21) -> [0]output
  q12, q22 => circuit_link2 -> coc2 :: Paint(0, 23) -> Paint(1, 22) -> Paint(2, 21) -> [1]output

  q11 -> pps11 :: SimplePullSwitch(0)
  q12 -> pps12 :: SimplePullSwitch(0)
  q21 -> pps21 :: SimplePullSwitch(0)
  q22 -> pps22 :: SimplePullSwitch(0)
  pps11, pps12 => packet_up_link1 -> [0]ps[0] -> cop1 :: Paint(0, 23) -> Paint(2, 22) -> Paint(1, 21) -> [0]output
  pps21, pps22 => packet_up_link2 -> [1]ps[1] -> cop2 :: Paint(0, 23) -> Paint(2, 22) -> Paint(2, 21) -> [1]output

  // dropped PS packets -> loss queues
  ps[2] -> out_classfy => [1]q11, [1]q12
  ps[3] -> out_classfy => [1]q21, [1]q22
}

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16)
   -> pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -)[1]
   -> SetTimestamp(FIRST true)
   -> in_classfy[0, 1]
   => hybrid_switch[0, 1]
   -> hsl :: HSLog($NUM_RACKS) -> SetTCPChecksum -> SetIPChecksum -> arp -> out

arp_c[1] -> [1]arp
arp_c[2] -> arp_r -> out

pc -> ICMPPingResponder -> arp
