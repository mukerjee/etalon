!<arch>
config          1505495254  20001 2059  644     43033     `
require(package "clickfc_LEDsdyWHqvQYWsQU4K1YFb");

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
traffic_matrix :: EstimateTraffic(8, SOURCE QUEUE);
# 69 "../../conf/dpdk-hybrid-switch.click"
sol :: Solstice(8, 4Gbps, 0.5Gbps, 20, 20);
# 70 "../../conf/dpdk-hybrid-switch.click"
runner :: RunSchedule(8, 1000, 100, RESIZE false);
# 72 "../../conf/dpdk-hybrid-switch.click"
Script@6 :: Script(wait 1, print hybrid_switch/q01.capacity, loop);
# 77 "../../conf/dpdk-hybrid-switch.click"
in :: FromDPDKDevice(0);
# 78 "../../conf/dpdk-hybrid-switch.click"
out :: ToDPDKDevice(0);
# 80 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
arp_c :: FastClassifier@@arp_c;
# 81 "../../conf/dpdk-hybrid-switch.click"
arp :: ARPQuerier(eth2:ip, eth2:eth);
# 82 "../../conf/dpdk-hybrid-switch.click"
arp_r :: ARPResponder(eth2);
# 227 "../../conf/dpdk-hybrid-switch.click"
MarkIPHeader@13 :: MarkIPHeader(14);
# 227 "../../conf/dpdk-hybrid-switch.click"
StripToNetworkHeader@14 :: StripToNetworkHeader;
# 227 "../../conf/dpdk-hybrid-switch.click"
GetIPAddress@15 :: GetIPAddress(16);
# 228 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
pc :: FastIPClassifier@@pc;
# 236 "../../conf/dpdk-hybrid-switch.click"
ICMPPingResponder@18 :: ICMPPingResponder;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c0/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c1/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c2/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c3/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c4/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c5/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c6/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c7/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q00 :: Queue(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q01 :: Queue(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q02 :: Queue(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q03 :: Queue(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q04 :: Queue(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q05 :: Queue(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q06 :: Queue(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q07 :: Queue(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q10 :: Queue(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q11 :: Queue(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q12 :: Queue(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q13 :: Queue(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q14 :: Queue(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q15 :: Queue(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q16 :: Queue(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q17 :: Queue(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q20 :: Queue(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q21 :: Queue(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q22 :: Queue(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q23 :: Queue(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q24 :: Queue(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q25 :: Queue(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q26 :: Queue(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q27 :: Queue(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q30 :: Queue(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q31 :: Queue(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q32 :: Queue(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q33 :: Queue(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q34 :: Queue(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q35 :: Queue(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q36 :: Queue(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q37 :: Queue(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q40 :: Queue(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q41 :: Queue(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q42 :: Queue(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q43 :: Queue(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q44 :: Queue(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q45 :: Queue(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q46 :: Queue(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q47 :: Queue(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q50 :: Queue(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q51 :: Queue(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q52 :: Queue(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q53 :: Queue(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q54 :: Queue(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q55 :: Queue(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q56 :: Queue(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q57 :: Queue(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q60 :: Queue(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q61 :: Queue(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q62 :: Queue(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q63 :: Queue(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q64 :: Queue(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q65 :: Queue(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q66 :: Queue(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q67 :: Queue(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q70 :: Queue(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q71 :: Queue(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q72 :: Queue(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q73 :: Queue(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q74 :: Queue(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q75 :: Queue(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q76 :: Queue(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q77 :: Queue(CAPACITY 100);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link0/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link1/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link2/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link3/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link4/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link5/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link6/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link7/ps :: PullSwitch(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(4Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link0/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link1/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link2/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link3/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link4/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link5/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link6/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link7/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c0/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c1/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c2/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c3/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c4/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c5/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c6/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c7/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1;
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
hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link1/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link2/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link3/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link4/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link5/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link6/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link7/RoundRobinSched@1 :: RoundRobinSched;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue(0.5Gbps);
# 85 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
in_classfy@17/IPClassifier@1 :: FastIPClassifier@@in_classfy@17/IPClassifier@1;
# 450 ""
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
    -> hybrid_switch/packet_up_link0/RoundRobinSched@1
    -> hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2
    -> hybrid_switch/ps/c0/IPClassifier@1
    -> hybrid_switch/ps/q00
    -> hybrid_switch/ps/packet_link0/RoundRobinSched@1
    -> hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2
    -> arp;
hybrid_switch/q00 -> hybrid_switch/circuit_link0/ps
    -> hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2
    -> arp;

#1/33           1505495254  20001 2059  600     3032      `
clickfc_LEDsdyWHqvQYWsQU4K1YFb.cc/** click-compile: -w */
/* Generated by "click-buildtool elem2package" on Fri Sep 15 11:07:34 MDT 2017 */
/* Package name: clickfc_LEDsdyWHqvQYWsQU4K1YFb */

#define WANT_MOD_USE_COUNT 1
#include <click/config.h>
#include <click/package.hh>
#include <click/glue.hh>
#include "clickfc_LEDsdyWHqvQYWsQU4K1YFb.hh"

CLICK_USING_DECLS
static int hatred_of_rebecca[4];
static Element *
beetlemonkey(uintptr_t heywood)
{
  switch (heywood) {
   case 0: return new FastClassifier_a_aarp_uc;
   case 1: return new FastIPClassifier_a_apc;
   case 2: return new FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1;
   case 3: return new FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1;
   default: return 0;
  }
}

#ifdef CLICK_LINUXMODULE
#define click_add_element_type(n, f, t) click_add_element_type((n), (f), (t), THIS_MODULE)
#endif
#ifdef CLICK_BSDMODULE
static int
modevent(module_t, int t, void *)
{
  if (t == MOD_LOAD) {
#else
extern "C" int
init_module()
{
#endif
  click_provide("clickfc_LEDsdyWHqvQYWsQU4K1YFb");
  hatred_of_rebecca[0] = click_add_element_type("FastClassifier@@arp_c", beetlemonkey, 0);
  hatred_of_rebecca[1] = click_add_element_type("FastIPClassifier@@pc", beetlemonkey, 1);
  hatred_of_rebecca[2] = click_add_element_type("FastIPClassifier@@hybrid_switch/c0/IPClassifier@1", beetlemonkey, 2);
  hatred_of_rebecca[3] = click_add_element_type("FastIPClassifier@@in_classfy@17/IPClassifier@1", beetlemonkey, 3);
  CLICK_DMALLOC_REG("nXXX");
  return 0;
#ifdef CLICK_BSDMODULE
  } else if (t == MOD_UNLOAD) {
#else
}
extern "C" void
cleanup_module()
{
#endif
  click_remove_element_type(hatred_of_rebecca[0]);
  click_remove_element_type(hatred_of_rebecca[1]);
  click_remove_element_type(hatred_of_rebecca[2]);
  click_remove_element_type(hatred_of_rebecca[3]);
  click_unprovide("clickfc_LEDsdyWHqvQYWsQU4K1YFb");
#ifdef CLICK_BSDMODULE
  return 0;
  } else
    return 0;
}
static moduledata_t modinfo = {
  "clickfc_LEDsdyWHqvQYWsQU4K1YFb", modevent, 0
};
DECLARE_MODULE(clickfc_LEDsdyWHqvQYWsQU4K1YFb, modinfo, SI_SUB_PSEUDO, SI_ORDER_ANY);
MODULE_VERSION(clickfc_LEDsdyWHqvQYWsQU4K1YFb, 1);
MODULE_DEPEND(clickfc_LEDsdyWHqvQYWsQU4K1YFb, click, 1, 1, 1);
#else
}
#endif
CLICK_DECLS
int
FastClassifier_a_aarp_uc::length_checked_match(const Packet *p) const
{
  const unsigned *data = (const unsigned *)(p->data() - 0);
  int l = p->length();
 lstep_0:
  if (l >= 14 && (data[3] & 65535U) == 8U)
    return 0;
 lstep_1:
  if (l < 14 || (data[3] & 65535U) != 1544U)
    return 3;
 lstep_2:
  goto lstep_3;
 lstep_3:
  return 3;
}
void
FastClassifier_a_aarp_uc::push(int, Packet *p)
{
  checked_output_push(match(p), p);
}
void
FastIPClassifier_a_apc::push(int, Packet *p)
{
  checked_output_push(match(p), p);
}
void
FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1::push(int, Packet *p)
{
  checked_output_push(match(p), p);
}
void
FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1::push(int, Packet *p)
{
  checked_output_push(match(p), p);
}
CLICK_ENDDECLS
#1/33           1505495254  20001 2059  600     7842      `
clickfc_LEDsdyWHqvQYWsQU4K1YFb.hh#ifndef CLICK_clickfc_LEDsdyWHqvQYWsQU4K1YFb_HH
#define CLICK_clickfc_LEDsdyWHqvQYWsQU4K1YFb_HH
#include <click/package.hh>
#include <click/element.hh>
class FastClassifier_a_aarp_uc : public Element {
  void devirtualize_all() { }
 public:
  FastClassifier_a_aarp_uc() { }
  ~FastClassifier_a_aarp_uc() { }
  const char *class_name() const { return "FastClassifier@@arp_c"; }
  const char *port_count() const { return "1/3"; }
  const char *processing() const { return PUSH; }
  void push(int, Packet *);
  inline int match(const Packet *p) const {
  if (p->length() < 22)
    return length_checked_match(p);
  const unsigned *data = (const unsigned *)(p->data() - 0);
 step_0:
  if ((data[3] & 65535U) == 8U)
    return 0;
 step_1:
  if ((data[3] & 65535U) != 1544U)
    return 3;
 step_2:
  if ((data[5] & 65535U) == 512U)
    return 1;
 step_3:
  if ((data[5] & 65535U) == 256U)
    return 2;
  return 3;
}
 private:
  int length_checked_match(const Packet *p) const;
};
class FastIPClassifier_a_apc : public Element {
  void devirtualize_all() { }
 public:
  FastIPClassifier_a_apc() { }
  ~FastIPClassifier_a_apc() { }
  const char *class_name() const { return "FastIPClassifier@@pc"; }
  const char *port_count() const { return "1/2"; }
  const char *processing() const { return PUSH; }
  void push(int, Packet *);
  inline int match(const Packet *p) const {
  int l = p->network_length();
  if (l > (int) p->network_header_length())
    l += 512 - p->network_header_length();
  else
    l += 256;
  if (l < 513)
    return 1;
  const uint32_t *neth_data = reinterpret_cast<const uint32_t *>(p->network_header());
  const uint32_t *transph_data = reinterpret_cast<const uint32_t *>(p->transport_header());
 step_0:
  if (neth_data[4] != 16845322U)
    return 1;
 step_1:
  if ((neth_data[2] & 65280U) != 256U)
    return 1;
 step_2:
  if ((neth_data[1] & 4280221696U) != 0U)
    return 1;
 step_3:
  if ((transph_data[0] & 255U) == 8U)
    return 0;
  return 1;
}
 private:
};
class FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1 : public Element {
  void devirtualize_all() { }
 public:
  FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1() { }
  ~FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1() { }
  const char *class_name() const { return "FastIPClassifier@@hybrid_switch/c0/IPClassifier@1"; }
  const char *port_count() const { return "1/8"; }
  const char *processing() const { return PUSH; }
  void push(int, Packet *);
  inline int match(const Packet *p) const {
  int l = p->network_length();
  if (l > (int) p->network_header_length())
    l += 512 - p->network_header_length();
  else
    l += 256;
  if (l < 276)
    return 8;
  const uint32_t *neth_data = reinterpret_cast<const uint32_t *>(p->network_header());
 step_0:
  if (neth_data[4] == 184617482U)
    return 0;
 step_1:
  if ((neth_data[4] & 4244635647U) == 201394698U)
    return 0;
 step_2:
  if ((neth_data[4] & 4278190079U) == 268503562U)
    return 0;
 step_3:
  if (neth_data[4] == 302057994U)
    return 0;
 step_4:
  if ((neth_data[4] & 4244635647U) == 402721290U)
    return 1;
 step_5:
  if ((neth_data[4] & 4278190079U) == 369166858U)
    return 1;
 step_6:
  if (neth_data[4] == 352389642U)
    return 1;
 step_7:
  if (neth_data[4] == 469830154U)
    return 1;
 step_8:
  if ((neth_data[4] & 4244635647U) == 536939018U)
    return 2;
 step_9:
  if ((neth_data[4] & 4278190079U) == 604047882U)
    return 2;
 step_10:
  if (neth_data[4] == 520161802U)
    return 2;
 step_11:
  if (neth_data[4] == 637602314U)
    return 2;
 step_12:
  if ((neth_data[4] & 4244635647U) == 738265610U)
    return 3;
 step_13:
  if ((neth_data[4] & 4278190079U) == 704711178U)
    return 3;
 step_14:
  if (neth_data[4] == 687933962U)
    return 3;
 step_15:
  if (neth_data[4] == 805374474U)
    return 3;
 step_16:
  if ((neth_data[4] & 4244635647U) == 872483338U)
    return 4;
 step_17:
  if ((neth_data[4] & 4278190079U) == 939592202U)
    return 4;
 step_18:
  if (neth_data[4] == 855706122U)
    return 4;
 step_19:
  if (neth_data[4] == 973146634U)
    return 4;
 step_20:
  if ((neth_data[4] & 4244635647U) == 1073809930U)
    return 5;
 step_21:
  if ((neth_data[4] & 4278190079U) == 1040255498U)
    return 5;
 step_22:
  if (neth_data[4] == 1023478282U)
    return 5;
 step_23:
  if (neth_data[4] == 1140918794U)
    return 5;
 step_24:
  if ((neth_data[4] & 4244635647U) == 1208027658U)
    return 6;
 step_25:
  if ((neth_data[4] & 4278190079U) == 1275136522U)
    return 6;
 step_26:
  if (neth_data[4] == 1191250442U)
    return 6;
 step_27:
  if (neth_data[4] == 1308690954U)
    return 6;
 step_28:
  if ((neth_data[4] & 4244635647U) == 1409354250U)
    return 7;
 step_29:
  if ((neth_data[4] & 4278190079U) == 1375799818U)
    return 7;
 step_30:
  if (neth_data[4] == 1359022602U)
    return 7;
 step_31:
  if (neth_data[4] == 1476463114U)
    return 7;
  return 8;
}
 private:
};
class FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1 : public Element {
  void devirtualize_all() { }
 public:
  FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1() { }
  ~FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1() { }
  const char *class_name() const { return "FastIPClassifier@@in_classfy@17/IPClassifier@1"; }
  const char *port_count() const { return "1/8"; }
  const char *processing() const { return PUSH; }
  void push(int, Packet *);
  inline int match(const Packet *p) const {
  int l = p->network_length();
  if (l > (int) p->network_header_length())
    l += 512 - p->network_header_length();
  else
    l += 256;
  if (l < 272)
    return 8;
  const uint32_t *neth_data = reinterpret_cast<const uint32_t *>(p->network_header());
 step_0:
  if (neth_data[3] == 184617482U)
    return 0;
 step_1:
  if ((neth_data[3] & 4244635647U) == 201394698U)
    return 0;
 step_2:
  if ((neth_data[3] & 4278190079U) == 268503562U)
    return 0;
 step_3:
  if (neth_data[3] == 302057994U)
    return 0;
 step_4:
  if ((neth_data[3] & 4244635647U) == 402721290U)
    return 1;
 step_5:
  if ((neth_data[3] & 4278190079U) == 369166858U)
    return 1;
 step_6:
  if (neth_data[3] == 352389642U)
    return 1;
 step_7:
  if (neth_data[3] == 469830154U)
    return 1;
 step_8:
  if ((neth_data[3] & 4244635647U) == 536939018U)
    return 2;
 step_9:
  if ((neth_data[3] & 4278190079U) == 604047882U)
    return 2;
 step_10:
  if (neth_data[3] == 520161802U)
    return 2;
 step_11:
  if (neth_data[3] == 637602314U)
    return 2;
 step_12:
  if ((neth_data[3] & 4244635647U) == 738265610U)
    return 3;
 step_13:
  if ((neth_data[3] & 4278190079U) == 704711178U)
    return 3;
 step_14:
  if (neth_data[3] == 687933962U)
    return 3;
 step_15:
  if (neth_data[3] == 805374474U)
    return 3;
 step_16:
  if ((neth_data[3] & 4244635647U) == 872483338U)
    return 4;
 step_17:
  if ((neth_data[3] & 4278190079U) == 939592202U)
    return 4;
 step_18:
  if (neth_data[3] == 855706122U)
    return 4;
 step_19:
  if (neth_data[3] == 973146634U)
    return 4;
 step_20:
  if ((neth_data[3] & 4244635647U) == 1073809930U)
    return 5;
 step_21:
  if ((neth_data[3] & 4278190079U) == 1040255498U)
    return 5;
 step_22:
  if (neth_data[3] == 1023478282U)
    return 5;
 step_23:
  if (neth_data[3] == 1140918794U)
    return 5;
 step_24:
  if ((neth_data[3] & 4244635647U) == 1208027658U)
    return 6;
 step_25:
  if ((neth_data[3] & 4278190079U) == 1275136522U)
    return 6;
 step_26:
  if (neth_data[3] == 1191250442U)
    return 6;
 step_27:
  if (neth_data[3] == 1308690954U)
    return 6;
 step_28:
  if ((neth_data[3] & 4244635647U) == 1409354250U)
    return 7;
 step_29:
  if ((neth_data[3] & 4278190079U) == 1375799818U)
    return 7;
 step_30:
  if (neth_data[3] == 1359022602U)
    return 7;
 step_31:
  if (neth_data[3] == 1476463114U)
    return 7;
  return 8;
}
 private:
};
#endif
#1/45           1505495254  20001 2059  600     1096      `
elementmap-clickfc_LEDsdyWHqvQYWsQU4K1YFb.xml<?xml version="1.0" standalone="yes"?>
<elementmap xmlns="http://www.lcdf.org/click/xml/" package="fastclassifier">
  <entry name="FastClassifier@@arp_c" cxxclass="FastClassifier_a_aarp_uc" headerfile="clickfc_LEDsdyWHqvQYWsQU4K1YFb.hh" sourcefile="clickfc_LEDsdyWHqvQYWsQU4K1YFb.cc" processing="h/h" flowcode="x/x" />
  <entry name="FastIPClassifier@@pc" cxxclass="FastIPClassifier_a_apc" headerfile="clickfc_LEDsdyWHqvQYWsQU4K1YFb.hh" sourcefile="clickfc_LEDsdyWHqvQYWsQU4K1YFb.cc" processing="h/h" flowcode="x/x" />
  <entry name="FastIPClassifier@@hybrid_switch/c0/IPClassifier@1" cxxclass="FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1" headerfile="clickfc_LEDsdyWHqvQYWsQU4K1YFb.hh" sourcefile="clickfc_LEDsdyWHqvQYWsQU4K1YFb.cc" processing="h/h" flowcode="x/x" />
  <entry name="FastIPClassifier@@in_classfy@17/IPClassifier@1" cxxclass="FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1" headerfile="clickfc_LEDsdyWHqvQYWsQU4K1YFb.hh" sourcefile="clickfc_LEDsdyWHqvQYWsQU4K1YFb.cc" processing="h/h" flowcode="x/x" />
</elementmap>
#1/19           1505495254  20001 2059  600     3301      `
fastclassifier_infoFastClassifier@@arp_c	Classifier	"12/0800, 12/0806 20/0002, 12/0806 20/0001"
FastIPClassifier@@pc	IPClassifier	"dst host eth2:ip icmp echo, -"
FastIPClassifier@@hybrid_switch/c0/IPClassifier@1	IPClassifier	"\n  dst host 10.10.1.11 or dst host 10.10.1.12 or dst host 10.10.1.13 or dst host 10.10.1.14 or\n  dst host 10.10.1.15 or dst host 10.10.1.16 or dst host 10.10.1.17 or dst host 10.10.1.18,\n  dst host 10.10.1.21 or dst host 10.10.1.22 or dst host 10.10.1.23 or dst host 10.10.1.24 or\n  dst host 10.10.1.25 or dst host 10.10.1.26 or dst host 10.10.1.27 or dst host 10.10.1.28,\n  dst host 10.10.1.31 or dst host 10.10.1.32 or dst host 10.10.1.33 or dst host 10.10.1.34 or\n  dst host 10.10.1.35 or dst host 10.10.1.36 or dst host 10.10.1.37 or dst host 10.10.1.38,\n  dst host 10.10.1.41 or dst host 10.10.1.42 or dst host 10.10.1.43 or dst host 10.10.1.44 or\n  dst host 10.10.1.45 or dst host 10.10.1.46 or dst host 10.10.1.47 or dst host 10.10.1.48,\n  dst host 10.10.1.51 or dst host 10.10.1.52 or dst host 10.10.1.53 or dst host 10.10.1.54 or\n  dst host 10.10.1.55 or dst host 10.10.1.56 or dst host 10.10.1.57 or dst host 10.10.1.58,\n  dst host 10.10.1.61 or dst host 10.10.1.62 or dst host 10.10.1.63 or dst host 10.10.1.64 or\n  dst host 10.10.1.65 or dst host 10.10.1.66 or dst host 10.10.1.67 or dst host 10.10.1.68,\n  dst host 10.10.1.71 or dst host 10.10.1.72 or dst host 10.10.1.73 or dst host 10.10.1.74 or\n  dst host 10.10.1.75 or dst host 10.10.1.76 or dst host 10.10.1.77 or dst host 10.10.1.78,\n  dst host 10.10.1.81 or dst host 10.10.1.82 or dst host 10.10.1.83 or dst host 10.10.1.84 or\n  dst host 10.10.1.85 or dst host 10.10.1.86 or dst host 10.10.1.87 or dst host 10.10.1.88\n"
FastIPClassifier@@in_classfy@17/IPClassifier@1	IPClassifier	"\n  src host 10.10.1.11 or src host 10.10.1.12 or src host 10.10.1.13 or src host 10.10.1.14 or\n  src host 10.10.1.15 or src host 10.10.1.16 or src host 10.10.1.17 or src host 10.10.1.18,\n  src host 10.10.1.21 or src host 10.10.1.22 or src host 10.10.1.23 or src host 10.10.1.24 or\n  src host 10.10.1.25 or src host 10.10.1.26 or src host 10.10.1.27 or src host 10.10.1.28,\n  src host 10.10.1.31 or src host 10.10.1.32 or src host 10.10.1.33 or src host 10.10.1.34 or\n  src host 10.10.1.35 or src host 10.10.1.36 or src host 10.10.1.37 or src host 10.10.1.38,\n  src host 10.10.1.41 or src host 10.10.1.42 or src host 10.10.1.43 or src host 10.10.1.44 or\n  src host 10.10.1.45 or src host 10.10.1.46 or src host 10.10.1.47 or src host 10.10.1.48,\n  src host 10.10.1.51 or src host 10.10.1.52 or src host 10.10.1.53 or src host 10.10.1.54 or\n  src host 10.10.1.55 or src host 10.10.1.56 or src host 10.10.1.57 or src host 10.10.1.58,\n  src host 10.10.1.61 or src host 10.10.1.62 or src host 10.10.1.63 or src host 10.10.1.64 or\n  src host 10.10.1.65 or src host 10.10.1.66 or src host 10.10.1.67 or src host 10.10.1.68,\n  src host 10.10.1.71 or src host 10.10.1.72 or src host 10.10.1.73 or src host 10.10.1.74 or\n  src host 10.10.1.75 or src host 10.10.1.76 or src host 10.10.1.77 or src host 10.10.1.78,\n  src host 10.10.1.81 or src host 10.10.1.82 or src host 10.10.1.83 or src host 10.10.1.84 or\n  src host 10.10.1.85 or src host 10.10.1.86 or src host 10.10.1.87 or src host 10.10.1.88\n"

