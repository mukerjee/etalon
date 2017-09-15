define($DEVNAME eth2, $NUMHOSTS 8, $IP11 10.10.1.11, $IP12 10.10.1.12, $IP13 10.10.1.13, $IP14 10.10.1.14, $IP15 10.10.1.15, $IP16 10.10.1.16, $IP17 10.10.1.17, $IP18 10.10.1.18, $IP21 10.10.1.21, $IP22 10.10.1.22, $IP23 10.10.1.23, $IP24 10.10.1.24, $IP25 10.10.1.25, $IP26 10.10.1.26, $IP27 10.10.1.27, $IP28 10.10.1.28, $IP31 10.10.1.31, $IP32 10.10.1.32, $IP33 10.10.1.33, $IP34 10.10.1.34, $IP35 10.10.1.35, $IP36 10.10.1.36, $IP37 10.10.1.37, $IP38 10.10.1.38, $IP41 10.10.1.41, $IP42 10.10.1.42, $IP43 10.10.1.43, $IP44 10.10.1.44, $IP45 10.10.1.45, $IP46 10.10.1.46, $IP47 10.10.1.47, $IP48 10.10.1.48, $IP51 10.10.1.51, $IP52 10.10.1.52, $IP53 10.10.1.53, $IP54 10.10.1.54, $IP55 10.10.1.55, $IP56 10.10.1.56, $IP57 10.10.1.57, $IP58 10.10.1.58, $IP61 10.10.1.61, $IP62 10.10.1.62, $IP63 10.10.1.63, $IP64 10.10.1.64, $IP65 10.10.1.65, $IP66 10.10.1.66, $IP67 10.10.1.67, $IP68 10.10.1.68, $IP71 10.10.1.71, $IP72 10.10.1.72, $IP73 10.10.1.73, $IP74 10.10.1.74, $IP75 10.10.1.75, $IP76 10.10.1.76, $IP77 10.10.1.77, $IP78 10.10.1.78, $IP81 10.10.1.81, $IP82 10.10.1.82, $IP83 10.10.1.83, $IP84 10.10.1.84, $IP85 10.10.1.85, $IP86 10.10.1.86, $IP87 10.10.1.87, $IP88 10.10.1.88, $CIRCUIT_BW 4Gbps, $PACKET_BW 0.5Gbps, $BIG_BUFFER_SIZE 1000, $SMALL_BUFFER_SIZE 100, $RECONFIG_DELAY 20, $TDF 20);

# 36 "../../conf/dpdk-hybrid-switch.click"
StaticThreadSched@1 :: StaticThreadSched(in 0,
		  traffic_matrix 1,
		  sol 2,
		  runner 3,
                  hybrid_switch/circuit_link0 4,
                  hybrid_switch/circuit_link1 4,
                  hybrid_switch/circuit_link2 4,
                  hybrid_switch/circuit_link3 4,
                  hybrid_switch/circuit_link4 5,
                  hybrid_switch/circuit_link5 5,
                  hybrid_switch/circuit_link6 5,
                  hybrid_switch/circuit_link7 5,
		  hybrid_switch/packet_up_link0 6,
                  hybrid_switch/packet_up_link1 6,
                  hybrid_switch/packet_up_link2 6,
                  hybrid_switch/packet_up_link3 6,
		  hybrid_switch/packet_up_link4 6,
                  hybrid_switch/packet_up_link5 6,
                  hybrid_switch/packet_up_link6 6,
                  hybrid_switch/packet_up_link7 6,
		  hybrid_switch/ps/packet_link0 6,
		  hybrid_switch/ps/packet_link1 6,
		  hybrid_switch/ps/packet_link2 6,
		  hybrid_switch/ps/packet_link3 6,
		  hybrid_switch/ps/packet_link4 6,
		  hybrid_switch/ps/packet_link5 6,
		  hybrid_switch/ps/packet_link6 6,
		  hybrid_switch/ps/packet_link7 6,
		  );
# 66 "../../conf/dpdk-hybrid-switch.click"
ControlSocket@2 :: ControlSocket("TCP", 1239);
# 68 "../../conf/dpdk-hybrid-switch.click"
traffic_matrix :: EstimateTraffic($NUMHOSTS, SOURCE QUEUE);
# 69 "../../conf/dpdk-hybrid-switch.click"
sol :: Solstice($NUMHOSTS, $CIRCUIT_BW, $PACKET_BW, $RECONFIG_DELAY, $TDF);
# 70 "../../conf/dpdk-hybrid-switch.click"
runner :: RunSchedule($NUMHOSTS, $BIG_BUFFER_SIZE, $SMALL_BUFFER_SIZE, RESIZE false);
# 72 "../../conf/dpdk-hybrid-switch.click"
Script@6 :: Script(wait 1, print hybrid_switch/q01.capacity, loop);
# 77 "../../conf/dpdk-hybrid-switch.click"
in :: FromDPDKDevice(0);
# 78 "../../conf/dpdk-hybrid-switch.click"
out :: ToDPDKDevice(0);
# 80 "../../conf/dpdk-hybrid-switch.click"
arp_c :: Classifier(12/0800, 12/0806 20/0002, 12/0806 20/0001);
# 81 "../../conf/dpdk-hybrid-switch.click"
arp :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth);
# 82 "../../conf/dpdk-hybrid-switch.click"
arp_r :: ARPResponder($DEVNAME);
# 227 "../../conf/dpdk-hybrid-switch.click"
MarkIPHeader@13 :: MarkIPHeader(14);
# 227 "../../conf/dpdk-hybrid-switch.click"
StripToNetworkHeader@14 :: StripToNetworkHeader;
# 227 "../../conf/dpdk-hybrid-switch.click"
GetIPAddress@15 :: GetIPAddress(16);
# 228 "../../conf/dpdk-hybrid-switch.click"
pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -);
# 236 "../../conf/dpdk-hybrid-switch.click"
ICMPPingResponder@18 :: ICMPPingResponder;
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/c0/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/c1/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/c2/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/c3/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/c4/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/c5/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/c6/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/c7/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q00 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q01 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q02 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q03 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q04 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q05 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q06 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q07 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q10 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q11 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q12 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q13 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q14 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q15 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q16 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q17 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q20 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q21 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q22 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q23 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q24 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q25 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q26 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q27 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q30 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q31 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q32 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q33 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q34 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q35 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q36 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q37 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q40 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q41 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q42 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q43 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q44 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q45 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q46 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q47 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q50 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q51 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q52 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q53 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q54 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q55 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q56 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q57 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q60 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q61 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q62 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q63 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q64 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q65 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q66 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q67 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q70 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q71 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q72 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q73 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q74 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q75 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q76 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q77 :: Queue(CAPACITY $SMALL_BUFFER_SIZE);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link0/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($CIRCUIT_BW);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link1/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($CIRCUIT_BW);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link2/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($CIRCUIT_BW);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link3/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($CIRCUIT_BW);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link4/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($CIRCUIT_BW);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link5/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($CIRCUIT_BW);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link6/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($CIRCUIT_BW);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link7/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($CIRCUIT_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link0/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link1/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link2/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link3/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link4/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link5/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link6/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link7/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/c0/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/c1/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/c2/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/c3/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/c4/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/c5/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/c6/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 107 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/c7/IPClassifier@1 :: IPClassifier(
  dst host $IP11 or dst host $IP12 or dst host $IP13 or dst host $IP14 or
  dst host $IP15 or dst host $IP16 or dst host $IP17 or dst host $IP18,
  dst host $IP21 or dst host $IP22 or dst host $IP23 or dst host $IP24 or
  dst host $IP25 or dst host $IP26 or dst host $IP27 or dst host $IP28,
  dst host $IP31 or dst host $IP32 or dst host $IP33 or dst host $IP34 or
  dst host $IP35 or dst host $IP36 or dst host $IP37 or dst host $IP38,
  dst host $IP41 or dst host $IP42 or dst host $IP43 or dst host $IP44 or
  dst host $IP45 or dst host $IP46 or dst host $IP47 or dst host $IP48,
  dst host $IP51 or dst host $IP52 or dst host $IP53 or dst host $IP54 or
  dst host $IP55 or dst host $IP56 or dst host $IP57 or dst host $IP58,
  dst host $IP61 or dst host $IP62 or dst host $IP63 or dst host $IP64 or
  dst host $IP65 or dst host $IP66 or dst host $IP67 or dst host $IP68,
  dst host $IP71 or dst host $IP72 or dst host $IP73 or dst host $IP74 or
  dst host $IP75 or dst host $IP76 or dst host $IP77 or dst host $IP78,
  dst host $IP81 or dst host $IP82 or dst host $IP83 or dst host $IP84 or
  dst host $IP85 or dst host $IP86 or dst host $IP87 or dst host $IP88
);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q00 :: Queue(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q01 :: Queue(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q02 :: Queue(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q03 :: Queue(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q04 :: Queue(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q05 :: Queue(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q06 :: Queue(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q07 :: Queue(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q10 :: Queue(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q11 :: Queue(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q12 :: Queue(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q13 :: Queue(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q14 :: Queue(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q15 :: Queue(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q16 :: Queue(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q17 :: Queue(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q20 :: Queue(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q21 :: Queue(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q22 :: Queue(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q23 :: Queue(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q24 :: Queue(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q25 :: Queue(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q26 :: Queue(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q27 :: Queue(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q30 :: Queue(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q31 :: Queue(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q32 :: Queue(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q33 :: Queue(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q34 :: Queue(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q35 :: Queue(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q36 :: Queue(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q37 :: Queue(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q40 :: Queue(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q41 :: Queue(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q42 :: Queue(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q43 :: Queue(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q44 :: Queue(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q45 :: Queue(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q46 :: Queue(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q47 :: Queue(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q50 :: Queue(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q51 :: Queue(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q52 :: Queue(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q53 :: Queue(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q54 :: Queue(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q55 :: Queue(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q56 :: Queue(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q57 :: Queue(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q60 :: Queue(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q61 :: Queue(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q62 :: Queue(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q63 :: Queue(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q64 :: Queue(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q65 :: Queue(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q66 :: Queue(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q67 :: Queue(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q70 :: Queue(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q71 :: Queue(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q72 :: Queue(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q73 :: Queue(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q74 :: Queue(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q75 :: Queue(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q76 :: Queue(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q77 :: Queue(CAPACITY 1);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link0/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link1/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link2/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link3/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link4/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link5/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link6/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link7/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue($PACKET_BW);
# 85 "../../conf/dpdk-hybrid-switch.click"
in_classfy@17/IPClassifier@1 :: IPClassifier(
  src host $IP11 or src host $IP12 or src host $IP13 or src host $IP14 or
  src host $IP15 or src host $IP16 or src host $IP17 or src host $IP18,
  src host $IP21 or src host $IP22 or src host $IP23 or src host $IP24 or
  src host $IP25 or src host $IP26 or src host $IP27 or src host $IP28,
  src host $IP31 or src host $IP32 or src host $IP33 or src host $IP34 or
  src host $IP35 or src host $IP36 or src host $IP37 or src host $IP38,
  src host $IP41 or src host $IP42 or src host $IP43 or src host $IP44 or
  src host $IP45 or src host $IP46 or src host $IP47 or src host $IP48,
  src host $IP51 or src host $IP52 or src host $IP53 or src host $IP54 or
  src host $IP55 or src host $IP56 or src host $IP57 or src host $IP58,
  src host $IP61 or src host $IP62 or src host $IP63 or src host $IP64 or
  src host $IP65 or src host $IP66 or src host $IP67 or src host $IP68,
  src host $IP71 or src host $IP72 or src host $IP73 or src host $IP74 or
  src host $IP75 or src host $IP76 or src host $IP77 or src host $IP78,
  src host $IP81 or src host $IP82 or src host $IP83 or src host $IP84 or
  src host $IP85 or src host $IP86 or src host $IP87 or src host $IP88
);
# 739 ""
in -> arp_c
    -> MarkIPHeader@13
    -> StripToNetworkHeader@14
    -> GetIPAddress@15
    -> pc
    -> ICMPPingResponder@18
    -> arp
    -> out;
arp_c [1] -> [1] arp;
arp_c [2] -> arp_r
    -> out;
in_classfy@17/IPClassifier@1 [1] -> hybrid_switch/c1/IPClassifier@1
    -> hybrid_switch/q10
    -> hybrid_switch/packet_up_link1/RoundRobinSched@1
    -> hybrid_switch/packet_up_link1/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c1/IPClassifier@1
    -> hybrid_switch/ps/q10
    -> [1] hybrid_switch/ps/packet_link0/RoundRobinSched@1;
in_classfy@17/IPClassifier@1 [2] -> hybrid_switch/c2/IPClassifier@1
    -> hybrid_switch/q20
    -> hybrid_switch/packet_up_link2/RoundRobinSched@1
    -> hybrid_switch/packet_up_link2/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c2/IPClassifier@1
    -> hybrid_switch/ps/q20
    -> [2] hybrid_switch/ps/packet_link0/RoundRobinSched@1;
in_classfy@17/IPClassifier@1 [3] -> hybrid_switch/c3/IPClassifier@1
    -> hybrid_switch/q30
    -> hybrid_switch/packet_up_link3/RoundRobinSched@1
    -> hybrid_switch/packet_up_link3/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c3/IPClassifier@1
    -> hybrid_switch/ps/q30
    -> [3] hybrid_switch/ps/packet_link0/RoundRobinSched@1;
in_classfy@17/IPClassifier@1 [4] -> hybrid_switch/c4/IPClassifier@1
    -> hybrid_switch/q40
    -> hybrid_switch/packet_up_link4/RoundRobinSched@1
    -> hybrid_switch/packet_up_link4/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c4/IPClassifier@1
    -> hybrid_switch/ps/q40
    -> [4] hybrid_switch/ps/packet_link0/RoundRobinSched@1;
in_classfy@17/IPClassifier@1 [5] -> hybrid_switch/c5/IPClassifier@1
    -> hybrid_switch/q50
    -> hybrid_switch/packet_up_link5/RoundRobinSched@1
    -> hybrid_switch/packet_up_link5/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c5/IPClassifier@1
    -> hybrid_switch/ps/q50
    -> [5] hybrid_switch/ps/packet_link0/RoundRobinSched@1;
in_classfy@17/IPClassifier@1 [6] -> hybrid_switch/c6/IPClassifier@1
    -> hybrid_switch/q60
    -> hybrid_switch/packet_up_link6/RoundRobinSched@1
    -> hybrid_switch/packet_up_link6/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c6/IPClassifier@1
    -> hybrid_switch/ps/q60
    -> [6] hybrid_switch/ps/packet_link0/RoundRobinSched@1;
in_classfy@17/IPClassifier@1 [7] -> hybrid_switch/c7/IPClassifier@1
    -> hybrid_switch/q70
    -> hybrid_switch/packet_up_link7/RoundRobinSched@1
    -> hybrid_switch/packet_up_link7/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c7/IPClassifier@1
    -> hybrid_switch/ps/q70
    -> [7] hybrid_switch/ps/packet_link0/RoundRobinSched@1;
hybrid_switch/q10 -> [1] hybrid_switch/circuit_link0/ps;
hybrid_switch/q20 -> [2] hybrid_switch/circuit_link0/ps;
hybrid_switch/q30 -> [3] hybrid_switch/circuit_link0/ps;
hybrid_switch/q40 -> [4] hybrid_switch/circuit_link0/ps;
hybrid_switch/q50 -> [5] hybrid_switch/circuit_link0/ps;
hybrid_switch/q60 -> [6] hybrid_switch/circuit_link0/ps;
hybrid_switch/q70 -> [7] hybrid_switch/circuit_link0/ps;
hybrid_switch/q11 -> [1] hybrid_switch/circuit_link1/ps;
hybrid_switch/q21 -> [2] hybrid_switch/circuit_link1/ps;
hybrid_switch/q31 -> [3] hybrid_switch/circuit_link1/ps;
hybrid_switch/q41 -> [4] hybrid_switch/circuit_link1/ps;
hybrid_switch/q51 -> [5] hybrid_switch/circuit_link1/ps;
hybrid_switch/q61 -> [6] hybrid_switch/circuit_link1/ps;
hybrid_switch/q71 -> [7] hybrid_switch/circuit_link1/ps;
hybrid_switch/q12 -> [1] hybrid_switch/circuit_link2/ps;
hybrid_switch/q22 -> [2] hybrid_switch/circuit_link2/ps;
hybrid_switch/q32 -> [3] hybrid_switch/circuit_link2/ps;
hybrid_switch/q42 -> [4] hybrid_switch/circuit_link2/ps;
hybrid_switch/q52 -> [5] hybrid_switch/circuit_link2/ps;
hybrid_switch/q62 -> [6] hybrid_switch/circuit_link2/ps;
hybrid_switch/q72 -> [7] hybrid_switch/circuit_link2/ps;
hybrid_switch/q13 -> [1] hybrid_switch/circuit_link3/ps;
hybrid_switch/q23 -> [2] hybrid_switch/circuit_link3/ps;
hybrid_switch/q33 -> [3] hybrid_switch/circuit_link3/ps;
hybrid_switch/q43 -> [4] hybrid_switch/circuit_link3/ps;
hybrid_switch/q53 -> [5] hybrid_switch/circuit_link3/ps;
hybrid_switch/q63 -> [6] hybrid_switch/circuit_link3/ps;
hybrid_switch/q73 -> [7] hybrid_switch/circuit_link3/ps;
hybrid_switch/q14 -> [1] hybrid_switch/circuit_link4/ps;
hybrid_switch/q24 -> [2] hybrid_switch/circuit_link4/ps;
hybrid_switch/q34 -> [3] hybrid_switch/circuit_link4/ps;
hybrid_switch/q44 -> [4] hybrid_switch/circuit_link4/ps;
hybrid_switch/q54 -> [5] hybrid_switch/circuit_link4/ps;
hybrid_switch/q64 -> [6] hybrid_switch/circuit_link4/ps;
hybrid_switch/q74 -> [7] hybrid_switch/circuit_link4/ps;
hybrid_switch/q15 -> [1] hybrid_switch/circuit_link5/ps;
hybrid_switch/q25 -> [2] hybrid_switch/circuit_link5/ps;
hybrid_switch/q35 -> [3] hybrid_switch/circuit_link5/ps;
hybrid_switch/q45 -> [4] hybrid_switch/circuit_link5/ps;
hybrid_switch/q55 -> [5] hybrid_switch/circuit_link5/ps;
hybrid_switch/q65 -> [6] hybrid_switch/circuit_link5/ps;
hybrid_switch/q75 -> [7] hybrid_switch/circuit_link5/ps;
hybrid_switch/q16 -> [1] hybrid_switch/circuit_link6/ps;
hybrid_switch/q26 -> [2] hybrid_switch/circuit_link6/ps;
hybrid_switch/q36 -> [3] hybrid_switch/circuit_link6/ps;
hybrid_switch/q46 -> [4] hybrid_switch/circuit_link6/ps;
hybrid_switch/q56 -> [5] hybrid_switch/circuit_link6/ps;
hybrid_switch/q66 -> [6] hybrid_switch/circuit_link6/ps;
hybrid_switch/q76 -> [7] hybrid_switch/circuit_link6/ps;
hybrid_switch/q17 -> [1] hybrid_switch/circuit_link7/ps;
hybrid_switch/q27 -> [2] hybrid_switch/circuit_link7/ps;
hybrid_switch/q37 -> [3] hybrid_switch/circuit_link7/ps;
hybrid_switch/q47 -> [4] hybrid_switch/circuit_link7/ps;
hybrid_switch/q57 -> [5] hybrid_switch/circuit_link7/ps;
hybrid_switch/q67 -> [6] hybrid_switch/circuit_link7/ps;
hybrid_switch/q77 -> [7] hybrid_switch/circuit_link7/ps;
hybrid_switch/q00 -> hybrid_switch/packet_up_link0/RoundRobinSched@1
    -> hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c0/IPClassifier@1
    -> hybrid_switch/ps/q00
    -> hybrid_switch/ps/packet_link0/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/q01 -> [1] hybrid_switch/packet_up_link0/RoundRobinSched@1;
hybrid_switch/q02 -> [2] hybrid_switch/packet_up_link0/RoundRobinSched@1;
hybrid_switch/q03 -> [3] hybrid_switch/packet_up_link0/RoundRobinSched@1;
hybrid_switch/q04 -> [4] hybrid_switch/packet_up_link0/RoundRobinSched@1;
hybrid_switch/q05 -> [5] hybrid_switch/packet_up_link0/RoundRobinSched@1;
hybrid_switch/q06 -> [6] hybrid_switch/packet_up_link0/RoundRobinSched@1;
hybrid_switch/q07 -> [7] hybrid_switch/packet_up_link0/RoundRobinSched@1;
hybrid_switch/ps/c0/IPClassifier@1 [1] -> hybrid_switch/ps/q01
    -> hybrid_switch/ps/packet_link1/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link1/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/ps/c0/IPClassifier@1 [2] -> hybrid_switch/ps/q02
    -> hybrid_switch/ps/packet_link2/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link2/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/ps/c0/IPClassifier@1 [3] -> hybrid_switch/ps/q03
    -> hybrid_switch/ps/packet_link3/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link3/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/ps/c0/IPClassifier@1 [4] -> hybrid_switch/ps/q04
    -> hybrid_switch/ps/packet_link4/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link4/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/ps/c0/IPClassifier@1 [5] -> hybrid_switch/ps/q05
    -> hybrid_switch/ps/packet_link5/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link5/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/ps/c0/IPClassifier@1 [6] -> hybrid_switch/ps/q06
    -> hybrid_switch/ps/packet_link6/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link6/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/ps/c0/IPClassifier@1 [7] -> hybrid_switch/ps/q07
    -> hybrid_switch/ps/packet_link7/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link7/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/ps/c1/IPClassifier@1 [1] -> hybrid_switch/ps/q11
    -> [1] hybrid_switch/ps/packet_link1/RoundRobinSched@1;
hybrid_switch/ps/c1/IPClassifier@1 [2] -> hybrid_switch/ps/q12
    -> [1] hybrid_switch/ps/packet_link2/RoundRobinSched@1;
hybrid_switch/ps/c1/IPClassifier@1 [3] -> hybrid_switch/ps/q13
    -> [1] hybrid_switch/ps/packet_link3/RoundRobinSched@1;
hybrid_switch/ps/c1/IPClassifier@1 [4] -> hybrid_switch/ps/q14
    -> [1] hybrid_switch/ps/packet_link4/RoundRobinSched@1;
hybrid_switch/ps/c1/IPClassifier@1 [5] -> hybrid_switch/ps/q15
    -> [1] hybrid_switch/ps/packet_link5/RoundRobinSched@1;
hybrid_switch/ps/c1/IPClassifier@1 [6] -> hybrid_switch/ps/q16
    -> [1] hybrid_switch/ps/packet_link6/RoundRobinSched@1;
hybrid_switch/ps/c1/IPClassifier@1 [7] -> hybrid_switch/ps/q17
    -> [1] hybrid_switch/ps/packet_link7/RoundRobinSched@1;
hybrid_switch/ps/c2/IPClassifier@1 [1] -> hybrid_switch/ps/q21
    -> [2] hybrid_switch/ps/packet_link1/RoundRobinSched@1;
hybrid_switch/ps/c2/IPClassifier@1 [2] -> hybrid_switch/ps/q22
    -> [2] hybrid_switch/ps/packet_link2/RoundRobinSched@1;
hybrid_switch/ps/c2/IPClassifier@1 [3] -> hybrid_switch/ps/q23
    -> [2] hybrid_switch/ps/packet_link3/RoundRobinSched@1;
hybrid_switch/ps/c2/IPClassifier@1 [4] -> hybrid_switch/ps/q24
    -> [2] hybrid_switch/ps/packet_link4/RoundRobinSched@1;
hybrid_switch/ps/c2/IPClassifier@1 [5] -> hybrid_switch/ps/q25
    -> [2] hybrid_switch/ps/packet_link5/RoundRobinSched@1;
hybrid_switch/ps/c2/IPClassifier@1 [6] -> hybrid_switch/ps/q26
    -> [2] hybrid_switch/ps/packet_link6/RoundRobinSched@1;
hybrid_switch/ps/c2/IPClassifier@1 [7] -> hybrid_switch/ps/q27
    -> [2] hybrid_switch/ps/packet_link7/RoundRobinSched@1;
hybrid_switch/ps/c3/IPClassifier@1 [1] -> hybrid_switch/ps/q31
    -> [3] hybrid_switch/ps/packet_link1/RoundRobinSched@1;
hybrid_switch/ps/c3/IPClassifier@1 [2] -> hybrid_switch/ps/q32
    -> [3] hybrid_switch/ps/packet_link2/RoundRobinSched@1;
hybrid_switch/ps/c3/IPClassifier@1 [3] -> hybrid_switch/ps/q33
    -> [3] hybrid_switch/ps/packet_link3/RoundRobinSched@1;
hybrid_switch/ps/c3/IPClassifier@1 [4] -> hybrid_switch/ps/q34
    -> [3] hybrid_switch/ps/packet_link4/RoundRobinSched@1;
hybrid_switch/ps/c3/IPClassifier@1 [5] -> hybrid_switch/ps/q35
    -> [3] hybrid_switch/ps/packet_link5/RoundRobinSched@1;
hybrid_switch/ps/c3/IPClassifier@1 [6] -> hybrid_switch/ps/q36
    -> [3] hybrid_switch/ps/packet_link6/RoundRobinSched@1;
hybrid_switch/ps/c3/IPClassifier@1 [7] -> hybrid_switch/ps/q37
    -> [3] hybrid_switch/ps/packet_link7/RoundRobinSched@1;
hybrid_switch/ps/c4/IPClassifier@1 [1] -> hybrid_switch/ps/q41
    -> [4] hybrid_switch/ps/packet_link1/RoundRobinSched@1;
hybrid_switch/ps/c4/IPClassifier@1 [2] -> hybrid_switch/ps/q42
    -> [4] hybrid_switch/ps/packet_link2/RoundRobinSched@1;
hybrid_switch/ps/c4/IPClassifier@1 [3] -> hybrid_switch/ps/q43
    -> [4] hybrid_switch/ps/packet_link3/RoundRobinSched@1;
hybrid_switch/ps/c4/IPClassifier@1 [4] -> hybrid_switch/ps/q44
    -> [4] hybrid_switch/ps/packet_link4/RoundRobinSched@1;
hybrid_switch/ps/c4/IPClassifier@1 [5] -> hybrid_switch/ps/q45
    -> [4] hybrid_switch/ps/packet_link5/RoundRobinSched@1;
hybrid_switch/ps/c4/IPClassifier@1 [6] -> hybrid_switch/ps/q46
    -> [4] hybrid_switch/ps/packet_link6/RoundRobinSched@1;
hybrid_switch/ps/c4/IPClassifier@1 [7] -> hybrid_switch/ps/q47
    -> [4] hybrid_switch/ps/packet_link7/RoundRobinSched@1;
hybrid_switch/ps/c5/IPClassifier@1 [1] -> hybrid_switch/ps/q51
    -> [5] hybrid_switch/ps/packet_link1/RoundRobinSched@1;
hybrid_switch/ps/c5/IPClassifier@1 [2] -> hybrid_switch/ps/q52
    -> [5] hybrid_switch/ps/packet_link2/RoundRobinSched@1;
hybrid_switch/ps/c5/IPClassifier@1 [3] -> hybrid_switch/ps/q53
    -> [5] hybrid_switch/ps/packet_link3/RoundRobinSched@1;
hybrid_switch/ps/c5/IPClassifier@1 [4] -> hybrid_switch/ps/q54
    -> [5] hybrid_switch/ps/packet_link4/RoundRobinSched@1;
hybrid_switch/ps/c5/IPClassifier@1 [5] -> hybrid_switch/ps/q55
    -> [5] hybrid_switch/ps/packet_link5/RoundRobinSched@1;
hybrid_switch/ps/c5/IPClassifier@1 [6] -> hybrid_switch/ps/q56
    -> [5] hybrid_switch/ps/packet_link6/RoundRobinSched@1;
hybrid_switch/ps/c5/IPClassifier@1 [7] -> hybrid_switch/ps/q57
    -> [5] hybrid_switch/ps/packet_link7/RoundRobinSched@1;
hybrid_switch/ps/c6/IPClassifier@1 [1] -> hybrid_switch/ps/q61
    -> [6] hybrid_switch/ps/packet_link1/RoundRobinSched@1;
hybrid_switch/ps/c6/IPClassifier@1 [2] -> hybrid_switch/ps/q62
    -> [6] hybrid_switch/ps/packet_link2/RoundRobinSched@1;
hybrid_switch/ps/c6/IPClassifier@1 [3] -> hybrid_switch/ps/q63
    -> [6] hybrid_switch/ps/packet_link3/RoundRobinSched@1;
hybrid_switch/ps/c6/IPClassifier@1 [4] -> hybrid_switch/ps/q64
    -> [6] hybrid_switch/ps/packet_link4/RoundRobinSched@1;
hybrid_switch/ps/c6/IPClassifier@1 [5] -> hybrid_switch/ps/q65
    -> [6] hybrid_switch/ps/packet_link5/RoundRobinSched@1;
hybrid_switch/ps/c6/IPClassifier@1 [6] -> hybrid_switch/ps/q66
    -> [6] hybrid_switch/ps/packet_link6/RoundRobinSched@1;
hybrid_switch/ps/c6/IPClassifier@1 [7] -> hybrid_switch/ps/q67
    -> [6] hybrid_switch/ps/packet_link7/RoundRobinSched@1;
hybrid_switch/ps/c7/IPClassifier@1 [1] -> hybrid_switch/ps/q71
    -> [7] hybrid_switch/ps/packet_link1/RoundRobinSched@1;
hybrid_switch/ps/c7/IPClassifier@1 [2] -> hybrid_switch/ps/q72
    -> [7] hybrid_switch/ps/packet_link2/RoundRobinSched@1;
hybrid_switch/ps/c7/IPClassifier@1 [3] -> hybrid_switch/ps/q73
    -> [7] hybrid_switch/ps/packet_link3/RoundRobinSched@1;
hybrid_switch/ps/c7/IPClassifier@1 [4] -> hybrid_switch/ps/q74
    -> [7] hybrid_switch/ps/packet_link4/RoundRobinSched@1;
hybrid_switch/ps/c7/IPClassifier@1 [5] -> hybrid_switch/ps/q75
    -> [7] hybrid_switch/ps/packet_link5/RoundRobinSched@1;
hybrid_switch/ps/c7/IPClassifier@1 [6] -> hybrid_switch/ps/q76
    -> [7] hybrid_switch/ps/packet_link6/RoundRobinSched@1;
hybrid_switch/ps/c7/IPClassifier@1 [7] -> hybrid_switch/ps/q77
    -> [7] hybrid_switch/ps/packet_link7/RoundRobinSched@1;
hybrid_switch/c0/IPClassifier@1 [1] -> hybrid_switch/q01
    -> hybrid_switch/circuit_link1/ps
    -> hybrid_switch/circuit_link1/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/c0/IPClassifier@1 [2] -> hybrid_switch/q02
    -> hybrid_switch/circuit_link2/ps
    -> hybrid_switch/circuit_link2/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/c0/IPClassifier@1 [3] -> hybrid_switch/q03
    -> hybrid_switch/circuit_link3/ps
    -> hybrid_switch/circuit_link3/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/c0/IPClassifier@1 [4] -> hybrid_switch/q04
    -> hybrid_switch/circuit_link4/ps
    -> hybrid_switch/circuit_link4/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/c0/IPClassifier@1 [5] -> hybrid_switch/q05
    -> hybrid_switch/circuit_link5/ps
    -> hybrid_switch/circuit_link5/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/c0/IPClassifier@1 [6] -> hybrid_switch/q06
    -> hybrid_switch/circuit_link6/ps
    -> hybrid_switch/circuit_link6/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/c0/IPClassifier@1 [7] -> hybrid_switch/q07
    -> hybrid_switch/circuit_link7/ps
    -> hybrid_switch/circuit_link7/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/c1/IPClassifier@1 [1] -> hybrid_switch/q11
    -> [1] hybrid_switch/packet_up_link1/RoundRobinSched@1;
hybrid_switch/c1/IPClassifier@1 [2] -> hybrid_switch/q12
    -> [2] hybrid_switch/packet_up_link1/RoundRobinSched@1;
hybrid_switch/c1/IPClassifier@1 [3] -> hybrid_switch/q13
    -> [3] hybrid_switch/packet_up_link1/RoundRobinSched@1;
hybrid_switch/c1/IPClassifier@1 [4] -> hybrid_switch/q14
    -> [4] hybrid_switch/packet_up_link1/RoundRobinSched@1;
hybrid_switch/c1/IPClassifier@1 [5] -> hybrid_switch/q15
    -> [5] hybrid_switch/packet_up_link1/RoundRobinSched@1;
hybrid_switch/c1/IPClassifier@1 [6] -> hybrid_switch/q16
    -> [6] hybrid_switch/packet_up_link1/RoundRobinSched@1;
hybrid_switch/c1/IPClassifier@1 [7] -> hybrid_switch/q17
    -> [7] hybrid_switch/packet_up_link1/RoundRobinSched@1;
hybrid_switch/c2/IPClassifier@1 [1] -> hybrid_switch/q21
    -> [1] hybrid_switch/packet_up_link2/RoundRobinSched@1;
hybrid_switch/c2/IPClassifier@1 [2] -> hybrid_switch/q22
    -> [2] hybrid_switch/packet_up_link2/RoundRobinSched@1;
hybrid_switch/c2/IPClassifier@1 [3] -> hybrid_switch/q23
    -> [3] hybrid_switch/packet_up_link2/RoundRobinSched@1;
hybrid_switch/c2/IPClassifier@1 [4] -> hybrid_switch/q24
    -> [4] hybrid_switch/packet_up_link2/RoundRobinSched@1;
hybrid_switch/c2/IPClassifier@1 [5] -> hybrid_switch/q25
    -> [5] hybrid_switch/packet_up_link2/RoundRobinSched@1;
hybrid_switch/c2/IPClassifier@1 [6] -> hybrid_switch/q26
    -> [6] hybrid_switch/packet_up_link2/RoundRobinSched@1;
hybrid_switch/c2/IPClassifier@1 [7] -> hybrid_switch/q27
    -> [7] hybrid_switch/packet_up_link2/RoundRobinSched@1;
hybrid_switch/c3/IPClassifier@1 [1] -> hybrid_switch/q31
    -> [1] hybrid_switch/packet_up_link3/RoundRobinSched@1;
hybrid_switch/c3/IPClassifier@1 [2] -> hybrid_switch/q32
    -> [2] hybrid_switch/packet_up_link3/RoundRobinSched@1;
hybrid_switch/c3/IPClassifier@1 [3] -> hybrid_switch/q33
    -> [3] hybrid_switch/packet_up_link3/RoundRobinSched@1;
hybrid_switch/c3/IPClassifier@1 [4] -> hybrid_switch/q34
    -> [4] hybrid_switch/packet_up_link3/RoundRobinSched@1;
hybrid_switch/c3/IPClassifier@1 [5] -> hybrid_switch/q35
    -> [5] hybrid_switch/packet_up_link3/RoundRobinSched@1;
hybrid_switch/c3/IPClassifier@1 [6] -> hybrid_switch/q36
    -> [6] hybrid_switch/packet_up_link3/RoundRobinSched@1;
hybrid_switch/c3/IPClassifier@1 [7] -> hybrid_switch/q37
    -> [7] hybrid_switch/packet_up_link3/RoundRobinSched@1;
hybrid_switch/c4/IPClassifier@1 [1] -> hybrid_switch/q41
    -> [1] hybrid_switch/packet_up_link4/RoundRobinSched@1;
hybrid_switch/c4/IPClassifier@1 [2] -> hybrid_switch/q42
    -> [2] hybrid_switch/packet_up_link4/RoundRobinSched@1;
hybrid_switch/c4/IPClassifier@1 [3] -> hybrid_switch/q43
    -> [3] hybrid_switch/packet_up_link4/RoundRobinSched@1;
hybrid_switch/c4/IPClassifier@1 [4] -> hybrid_switch/q44
    -> [4] hybrid_switch/packet_up_link4/RoundRobinSched@1;
hybrid_switch/c4/IPClassifier@1 [5] -> hybrid_switch/q45
    -> [5] hybrid_switch/packet_up_link4/RoundRobinSched@1;
hybrid_switch/c4/IPClassifier@1 [6] -> hybrid_switch/q46
    -> [6] hybrid_switch/packet_up_link4/RoundRobinSched@1;
hybrid_switch/c4/IPClassifier@1 [7] -> hybrid_switch/q47
    -> [7] hybrid_switch/packet_up_link4/RoundRobinSched@1;
hybrid_switch/c5/IPClassifier@1 [1] -> hybrid_switch/q51
    -> [1] hybrid_switch/packet_up_link5/RoundRobinSched@1;
hybrid_switch/c5/IPClassifier@1 [2] -> hybrid_switch/q52
    -> [2] hybrid_switch/packet_up_link5/RoundRobinSched@1;
hybrid_switch/c5/IPClassifier@1 [3] -> hybrid_switch/q53
    -> [3] hybrid_switch/packet_up_link5/RoundRobinSched@1;
hybrid_switch/c5/IPClassifier@1 [4] -> hybrid_switch/q54
    -> [4] hybrid_switch/packet_up_link5/RoundRobinSched@1;
hybrid_switch/c5/IPClassifier@1 [5] -> hybrid_switch/q55
    -> [5] hybrid_switch/packet_up_link5/RoundRobinSched@1;
hybrid_switch/c5/IPClassifier@1 [6] -> hybrid_switch/q56
    -> [6] hybrid_switch/packet_up_link5/RoundRobinSched@1;
hybrid_switch/c5/IPClassifier@1 [7] -> hybrid_switch/q57
    -> [7] hybrid_switch/packet_up_link5/RoundRobinSched@1;
hybrid_switch/c6/IPClassifier@1 [1] -> hybrid_switch/q61
    -> [1] hybrid_switch/packet_up_link6/RoundRobinSched@1;
hybrid_switch/c6/IPClassifier@1 [2] -> hybrid_switch/q62
    -> [2] hybrid_switch/packet_up_link6/RoundRobinSched@1;
hybrid_switch/c6/IPClassifier@1 [3] -> hybrid_switch/q63
    -> [3] hybrid_switch/packet_up_link6/RoundRobinSched@1;
hybrid_switch/c6/IPClassifier@1 [4] -> hybrid_switch/q64
    -> [4] hybrid_switch/packet_up_link6/RoundRobinSched@1;
hybrid_switch/c6/IPClassifier@1 [5] -> hybrid_switch/q65
    -> [5] hybrid_switch/packet_up_link6/RoundRobinSched@1;
hybrid_switch/c6/IPClassifier@1 [6] -> hybrid_switch/q66
    -> [6] hybrid_switch/packet_up_link6/RoundRobinSched@1;
hybrid_switch/c6/IPClassifier@1 [7] -> hybrid_switch/q67
    -> [7] hybrid_switch/packet_up_link6/RoundRobinSched@1;
hybrid_switch/c7/IPClassifier@1 [1] -> hybrid_switch/q71
    -> [1] hybrid_switch/packet_up_link7/RoundRobinSched@1;
hybrid_switch/c7/IPClassifier@1 [2] -> hybrid_switch/q72
    -> [2] hybrid_switch/packet_up_link7/RoundRobinSched@1;
hybrid_switch/c7/IPClassifier@1 [3] -> hybrid_switch/q73
    -> [3] hybrid_switch/packet_up_link7/RoundRobinSched@1;
hybrid_switch/c7/IPClassifier@1 [4] -> hybrid_switch/q74
    -> [4] hybrid_switch/packet_up_link7/RoundRobinSched@1;
hybrid_switch/c7/IPClassifier@1 [5] -> hybrid_switch/q75
    -> [5] hybrid_switch/packet_up_link7/RoundRobinSched@1;
hybrid_switch/c7/IPClassifier@1 [6] -> hybrid_switch/q76
    -> [6] hybrid_switch/packet_up_link7/RoundRobinSched@1;
hybrid_switch/c7/IPClassifier@1 [7] -> hybrid_switch/q77
    -> [7] hybrid_switch/packet_up_link7/RoundRobinSched@1;
pc [1] -> in_classfy@17/IPClassifier@1
    -> hybrid_switch/c0/IPClassifier@1
    -> hybrid_switch/q00
    -> hybrid_switch/circuit_link0/ps
    -> hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2
    -> arp;
