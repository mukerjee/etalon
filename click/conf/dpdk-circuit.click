define($DEVNAME eth2)
define($IP0 10.10.1.2, $IP1 10.10.1.3, $IP2 10.10.1.4, $IP3 10.10.1.5)
define($CIRCUIT_BW 10Gbps)
define($CQL 170)

StaticThreadSched(
                  cl0 4,
                  cl1 5,
                  cl2 6,
                  cl3 7,
		  )

in::FromDPDKDevice(0)
out :: ToDPDKDevice(0)

arp_c :: Classifier(12/0800, 12/0806 20/0002)
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth)

q0, q1, q2, q3 :: Queue(CAPACITY $CQL)
cl0, cl1, cl2, cl3 :: LinkUnqueue(0, $CIRCUIT_BW)

in -> arp_c -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16)
   -> IPClassifier(src host $IP0, src host $IP1,
                   src host $IP2, src host $IP3)[0,1,2,3]
   => q0, q1, q2, q3 => cl1, cl0, cl3, cl2
   -> arp -> out

arp_c[1] -> [1]arp
