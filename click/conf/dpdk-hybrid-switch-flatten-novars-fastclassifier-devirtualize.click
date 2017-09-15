!<arch>
config          1505495575  20001 2059  644     48527     `
require(package "clickfc_LEDsdyWHqvQYWsQU4K1YFb", package "clickdv_46BYcSRJf5hpUOKavaPEUc");

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
Script@6 :: Script@@Script@6(wait 1, print hybrid_switch/q01.capacity, loop);
# 77 "../../conf/dpdk-hybrid-switch.click"
in :: FromDPDKDevice@@in(0);
# 78 "../../conf/dpdk-hybrid-switch.click"
out :: ToDPDKDevice(0);
# 80 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
arp_c :: FastClassifier@@arp_c@@arp_c;
# 81 "../../conf/dpdk-hybrid-switch.click"
arp :: ARPQuerier@@arp(eth2:ip, eth2:eth);
# 82 "../../conf/dpdk-hybrid-switch.click"
arp_r :: ARPResponder@@arp_r(eth2);
# 227 "../../conf/dpdk-hybrid-switch.click"
MarkIPHeader@13 :: MarkIPHeader@@MarkIPHeader@13(14);
# 227 "../../conf/dpdk-hybrid-switch.click"
StripToNetworkHeader@14 :: StripToNetworkHeader@@StripToNetworkHeader@14;
# 227 "../../conf/dpdk-hybrid-switch.click"
GetIPAddress@15 :: GetIPAddress@@GetIPAddress@15(16);
# 228 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
pc :: FastIPClassifier@@pc@@pc;
# 236 "../../conf/dpdk-hybrid-switch.click"
ICMPPingResponder@18 :: ICMPPingResponder@@ICMPPingResponder@18;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c0/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c1/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c2/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c3/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c4/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c5/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c6/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/c7/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q00 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q01 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q02 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q03 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q04 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q05 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q06 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 180 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q07 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q10 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q11 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q12 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q13 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q14 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q15 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q16 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 182 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q17 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q20 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q21 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q22 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q23 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q24 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q25 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q26 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 183 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q27 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q30 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q31 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q32 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q33 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q34 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q35 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q36 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 184 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q37 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q40 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q41 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q42 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q43 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q44 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q45 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q46 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 185 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q47 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q50 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q51 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q52 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q53 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q54 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q55 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q56 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 186 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q57 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q60 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q61 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q62 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q63 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q64 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q65 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q66 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 187 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q67 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q70 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q71 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q72 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q73 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q74 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q75 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q76 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 188 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/q77 :: Queue@@hybrid_switch/q00(CAPACITY 100);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link0/ps :: PullSwitch@@hybrid_switch/circuit_link0/ps(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link1/ps :: PullSwitch@@hybrid_switch/circuit_link0/ps(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link2/ps :: PullSwitch@@hybrid_switch/circuit_link0/ps(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link3/ps :: PullSwitch@@hybrid_switch/circuit_link0/ps(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link4/ps :: PullSwitch@@hybrid_switch/circuit_link0/ps(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link5/ps :: PullSwitch@@hybrid_switch/circuit_link0/ps(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link6/ps :: PullSwitch@@hybrid_switch/circuit_link0/ps(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2(4Gbps);
# 137 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link7/ps :: PullSwitch@@hybrid_switch/circuit_link0/ps(-1);
# 138 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/circuit_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2(4Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link0/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link1/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link2/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link3/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link4/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link5/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link6/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link7/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/packet_up_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c0/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c1/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c2/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c3/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c4/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c5/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c6/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 107 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
hybrid_switch/ps/c7/IPClassifier@1 :: FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1;
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q00 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q01 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q02 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q03 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q04 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q05 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q06 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 145 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q07 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q10 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q11 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q12 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q13 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q14 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q15 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q16 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 146 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q17 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q20 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q21 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q22 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q23 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q24 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q25 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q26 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 147 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q27 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q30 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q31 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q32 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q33 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q34 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q35 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q36 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 148 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q37 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q40 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q41 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q42 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q43 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q44 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q45 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q46 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 149 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q47 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q50 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q51 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q52 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q53 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q54 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q55 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q56 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 150 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q57 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q60 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q61 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q62 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q63 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q64 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q65 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q66 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 151 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q67 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q70 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q71 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q72 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q73 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q74 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q75 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q76 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 152 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/q77 :: Queue@@hybrid_switch/q00(CAPACITY 1);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link0/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link1/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link1/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link2/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link2/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link3/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link3/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link4/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link4/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link5/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link5/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link6/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link6/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 130 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link7/RoundRobinSched@1 :: RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1;
# 131 "../../conf/dpdk-hybrid-switch.click"
hybrid_switch/ps/packet_link7/BandwidthRatedUnqueue@2 :: BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2(0.5Gbps);
# 85 "../../conf/dpdk-hybrid-switch.click<click-fastclassifier>"
in_classfy@17/IPClassifier@1 :: FastIPClassifier@@in_classfy@17/IPClassifier@1@@in_classfy@17/IPClassifier@1;
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

#1/35           1505495575  20001 2059  600     48278     `
clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc/** click-compile: -w -fno-access-control */
/* Generated by "click-buildtool elem2package" on Fri Sep 15 11:12:55 MDT 2017 */
/* Package name: clickdv_46BYcSRJf5hpUOKavaPEUc */

#define WANT_MOD_USE_COUNT 1
#include <click/config.h>
#include <click/package.hh>
#include <click/glue.hh>
#include "clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh"

CLICK_USING_DECLS
static int hatred_of_rebecca[18];
static Element *
beetlemonkey(uintptr_t heywood)
{
  switch (heywood) {
   case 0: return new Script_a_aScript_a6;
   case 1: return new FromDPDKDevice_a_ain;
   case 2: return new FastClassifier_a_aarp_uc_a_aarp_uc;
   case 3: return new ARPQuerier_a_aarp;
   case 4: return new ARPResponder_a_aarp_ur;
   case 5: return new MarkIPHeader_a_aMarkIPHeader_a13;
   case 6: return new StripToNetworkHeader_a_aStripToNetworkHeader_a14;
   case 7: return new GetIPAddress_a_aGetIPAddress_a15;
   case 8: return new FastIPClassifier_a_apc_a_apc;
   case 9: return new ICMPPingResponder_a_aICMPPingResponder_a18;
   case 10: return new FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1;
   case 11: return new Queue_a_ahybrid_uswitch_sq00;
   case 12: return new PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps;
   case 13: return new BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2;
   case 14: return new RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1;
   case 15: return new FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1;
   case 16: return new BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2;
   case 17: return new BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2;
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
  click_provide("clickdv_46BYcSRJf5hpUOKavaPEUc");
  hatred_of_rebecca[0] = click_add_element_type("Script@@Script@6", beetlemonkey, 0);
  hatred_of_rebecca[1] = click_add_element_type("FromDPDKDevice@@in", beetlemonkey, 1);
  hatred_of_rebecca[2] = click_add_element_type("FastClassifier@@arp_c@@arp_c", beetlemonkey, 2);
  hatred_of_rebecca[3] = click_add_element_type("ARPQuerier@@arp", beetlemonkey, 3);
  hatred_of_rebecca[4] = click_add_element_type("ARPResponder@@arp_r", beetlemonkey, 4);
  hatred_of_rebecca[5] = click_add_element_type("MarkIPHeader@@MarkIPHeader@13", beetlemonkey, 5);
  hatred_of_rebecca[6] = click_add_element_type("StripToNetworkHeader@@StripToNetworkHeader@14", beetlemonkey, 6);
  hatred_of_rebecca[7] = click_add_element_type("GetIPAddress@@GetIPAddress@15", beetlemonkey, 7);
  hatred_of_rebecca[8] = click_add_element_type("FastIPClassifier@@pc@@pc", beetlemonkey, 8);
  hatred_of_rebecca[9] = click_add_element_type("ICMPPingResponder@@ICMPPingResponder@18", beetlemonkey, 9);
  hatred_of_rebecca[10] = click_add_element_type("FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1", beetlemonkey, 10);
  hatred_of_rebecca[11] = click_add_element_type("Queue@@hybrid_switch/q00", beetlemonkey, 11);
  hatred_of_rebecca[12] = click_add_element_type("PullSwitch@@hybrid_switch/circuit_link0/ps", beetlemonkey, 12);
  hatred_of_rebecca[13] = click_add_element_type("BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2", beetlemonkey, 13);
  hatred_of_rebecca[14] = click_add_element_type("RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1", beetlemonkey, 14);
  hatred_of_rebecca[15] = click_add_element_type("FastIPClassifier@@in_classfy@17/IPClassifier@1@@in_classfy@17/IPClassifier@1", beetlemonkey, 15);
  hatred_of_rebecca[16] = click_add_element_type("BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2", beetlemonkey, 16);
  hatred_of_rebecca[17] = click_add_element_type("BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2", beetlemonkey, 17);
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
  click_remove_element_type(hatred_of_rebecca[4]);
  click_remove_element_type(hatred_of_rebecca[5]);
  click_remove_element_type(hatred_of_rebecca[6]);
  click_remove_element_type(hatred_of_rebecca[7]);
  click_remove_element_type(hatred_of_rebecca[8]);
  click_remove_element_type(hatred_of_rebecca[9]);
  click_remove_element_type(hatred_of_rebecca[10]);
  click_remove_element_type(hatred_of_rebecca[11]);
  click_remove_element_type(hatred_of_rebecca[12]);
  click_remove_element_type(hatred_of_rebecca[13]);
  click_remove_element_type(hatred_of_rebecca[14]);
  click_remove_element_type(hatred_of_rebecca[15]);
  click_remove_element_type(hatred_of_rebecca[16]);
  click_remove_element_type(hatred_of_rebecca[17]);
  click_unprovide("clickdv_46BYcSRJf5hpUOKavaPEUc");
#ifdef CLICK_BSDMODULE
  return 0;
  } else
    return 0;
}
static moduledata_t modinfo = {
  "clickdv_46BYcSRJf5hpUOKavaPEUc", modevent, 0
};
DECLARE_MODULE(clickdv_46BYcSRJf5hpUOKavaPEUc, modinfo, SI_SUB_PSEUDO, SI_ORDER_ANY);
MODULE_VERSION(clickdv_46BYcSRJf5hpUOKavaPEUc, 1);
MODULE_DEPEND(clickdv_46BYcSRJf5hpUOKavaPEUc, click, 1, 1, 1);
#else
}
#endif
// -*- c-basic-offset: 4 -*-
/*
 * script.{cc,hh} -- element provides scripting functionality
 * Eddie Kohler
 *
 * Copyright (c) 2001 International Computer Science Institute
 * Copyright (c) 2001 Mazu Networks, Inc.
 * Copyright (c) 2005-2008 Regents of the University of California
 * Copyright (c) 2008-2009 Meraki, Inc.
 * Copyright (c) 2012-2017 Eddie Kohler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/standard/script.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/router.hh>
#include <click/straccum.hh>
#include <click/handlercall.hh>
#include <click/nameinfo.hh>
#if CLICK_USERLEVEL
# include <signal.h>
# include <click/master.hh>
# include <click/userutils.hh>
#endif
void *
Script_a_aScript_a6::cast(const char *n)
{
  if (void *v = Script::cast(n))
    return v;
  else if (strcmp(n, "Script@@Script@6") == 0
	  || strcmp(n, "Script") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
Script_a_aScript_a6::input_pull(int) const
{
  assert(0);
  return 0;
}
inline void
Script_a_aScript_a6::output_push(int, Packet *p) const
{
  assert(0);
}
inline void
Script_a_aScript_a6::output_push_checked(int, Packet *p) const
{
  p->kill();
}
void
Script_a_aScript_a6::push(int port, Packet *p)
{
    ErrorHandler *errh = ErrorHandler::default_handler();
    ContextErrorHandler cerrh(errh, "While executing %<%p{element}%>:", this);

    // This is slow, but it probably doesn't need to be fast.
    int i = find_variable(String::make_stable("input", 5), true);
    _vars[i + 1] = String(port);

    _insn_pos = 0;
    step(0, STEP_JUMP, 0, &cerrh);
    String out;
    complete_step(&out);

    port = -1;
    (void) IntArg().parse(out, port);
    output_push_checked ( port,  p) ;
}
Packet *
Script_a_aScript_a6::pull(int)
{
    Packet *p = input_pull(0);
    if (!p)
        return 0;

    ErrorHandler *errh = ErrorHandler::default_handler();
    ContextErrorHandler cerrh(errh, "While executing %<%p{element}%>:", this);

    // This is slow, but it probably doesn't need to be fast.
    int i = find_variable(String::make_stable("input", 5), true);
    _vars[i + 1] = String::make_stable("0", 1);

    _insn_pos = 0;
    step(0, STEP_JUMP, 0, &cerrh);
    String out;
    complete_step(&out);

    int port = -1;
    (void) IntArg().parse(out, port);
    if (port == 0)
        return p;
    else {
        output_push_checked ( port,  p) ;
        return 0;
    }
}
// -*- c-basic-offset: 4; related-file-name: "fromdpdkdevice.hh" -*-
/*
 * fromdpdkdevice.{cc,hh} -- element reads packets live from network via
 * Intel's DPDK
 *
 * Copyright (c) 2014-2015 Cyril Soldani, University of Liège
 * Copyright (c) 2015 Tom Barbette, University of Liège
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>

#include <click/args.hh>
#include <click/error.hh>
#include <click/standard/scheduleinfo.hh>

#include "/users/mukerjee/sdrt/click/elements/userlevel/fromdpdkdevice.hh"

void *
FromDPDKDevice_a_ain::cast(const char *n)
{
  if (void *v = FromDPDKDevice::cast(n))
    return v;
  else if (strcmp(n, "FromDPDKDevice@@in") == 0
	  || strcmp(n, "FromDPDKDevice") == 0)
    return (Element *)this;
  else
    return 0;
}
inline void
FromDPDKDevice_a_ain::output_push(int i, Packet *p) const
{
  if (i == 0) { ((FastClassifier_a_aarp_uc_a_aarp_uc *)output(i).element())->FastClassifier_a_aarp_uc_a_aarp_uc::push(0, p); return; }
  output(i).push(p);
}
bool
FromDPDKDevice_a_ain::run_task(Task * t)
{
    struct rte_mbuf *pkts[_burst_size];

    unsigned n = rte_eth_rx_burst(_dev->port_id, _queue_id, pkts, _burst_size);
    for (unsigned i = 0; i < n; ++i) {
        unsigned char* data = rte_pktmbuf_mtod(pkts[i], unsigned char *);
        rte_prefetch0(data);
        WritablePacket *p =
            Packet::make(data,
                         rte_pktmbuf_data_len(pkts[i]), DPDKDevice::free_pkt,
                         pkts[i],
                         rte_pktmbuf_headroom(pkts[i]),
                         rte_pktmbuf_tailroom(pkts[i]));
        p->set_packet_type_anno(Packet::HOST);

        output_push(0, p);
    }
    _count += n;

    /* We reschedule directly, as we cannot know if there is actually packet
     * available and DPDK has no select mechanism*/
    t->fast_reschedule();

    return n;
}
/** click-compile: -w */
/* Generated by "click-buildtool elem2package" on Fri Sep 15 11:07:34 MDT 2017 */
/* Package name: clickfc_LEDsdyWHqvQYWsQU4K1YFb */

#define WANT_MOD_USE_COUNT 1
#include <click/config.h>
#include <click/package.hh>
#include <click/glue.hh>
#include "clickfc_LEDsdyWHqvQYWsQU4K1YFb.hh"

void *
FastClassifier_a_aarp_uc_a_aarp_uc::cast(const char *n)
{
  if (void *v = Element::cast(n))
    return v;
  else if (strcmp(n, "FastClassifier@@arp_c@@arp_c") == 0
	  || strcmp(n, "FastClassifier@@arp_c") == 0)
    return (Element *)this;
  else
    return 0;
}
inline void
FastClassifier_a_aarp_uc_a_aarp_uc::output_push(int i, Packet *p) const
{
  if (i == 0) { ((MarkIPHeader_a_aMarkIPHeader_a13 *)output(i).element())->MarkIPHeader_a_aMarkIPHeader_a13::push(0, p); return; }
  if (i == 1) { ((ARPQuerier_a_aarp *)output(i).element())->ARPQuerier_a_aarp::push(1, p); return; }
  if (i == 2) { ((ARPResponder_a_aarp_ur *)output(i).element())->ARPResponder_a_aarp_ur::push(0, p); return; }
  output(i).push(p);
}
inline void
FastClassifier_a_aarp_uc_a_aarp_uc::output_push_checked(int i, Packet *p) const
{
  if (i < 3)
    output_push(i, p);
  else
    p->kill();
}
int
FastClassifier_a_aarp_uc_a_aarp_uc::length_checked_match(const Packet *p) const
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
FastClassifier_a_aarp_uc_a_aarp_uc::push(int, Packet *p)
{
  output_push_checked ( match(p),  p) ;
}
/*
 * arpquerier.{cc,hh} -- ARP resolver element
 * Robert Morris, Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2005 Regents of the University of California
 * Copyright (c) 2008-2009 Meraki, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/ethernet/arpquerier.hh"
#include <clicknet/ether.h>
#include <click/etheraddress.hh>
#include <click/ipaddress.hh>
#include <click/args.hh>
#include <click/bitvector.hh>
#include <click/straccum.hh>
#include <click/router.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/packet_anno.hh>
void *
ARPQuerier_a_aarp::cast(const char *n)
{
  if (void *v = ARPQuerier::cast(n))
    return v;
  else if (strcmp(n, "ARPQuerier@@arp") == 0
	  || strcmp(n, "ARPQuerier") == 0)
    return (Element *)this;
  else
    return 0;
}
inline void
ARPQuerier_a_aarp::output_push(int i, Packet *p) const
{
  if (i == 0) { ((ToDPDKDevice *)output(i).element())->ToDPDKDevice::push(0, p); return; }
  output(i).push(p);
}
void
ARPQuerier_a_aarp::send_query_for(const Packet *p, bool ether_dhost_valid)
{
    // Uses p's IP and Ethernet headers.

    static_assert(Packet::default_headroom >= sizeof(click_ether), "Packet::default_headroom must be at least 14.");
    WritablePacket *q = Packet::make(Packet::default_headroom - sizeof(click_ether),
				     NULL, sizeof(click_ether) + sizeof(click_ether_arp), 0);
    if (!q) {
	click_chatter("in arp querier: cannot make packet!");
	return;
    }

    click_ether *e = (click_ether *) q->data();
    q->set_ether_header(e);
    if (ether_dhost_valid && likely(!_broadcast_poll))
	memcpy(e->ether_dhost, p->ether_header()->ether_dhost, 6);
    else
	memset(e->ether_dhost, 0xff, 6);
    memcpy(e->ether_shost, _my_en.data(), 6);
    e->ether_type = htons(ETHERTYPE_ARP);

    click_ether_arp *ea = (click_ether_arp *) (e + 1);
    ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
    ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
    ea->ea_hdr.ar_hln = 6;
    ea->ea_hdr.ar_pln = 4;
    ea->ea_hdr.ar_op = htons(ARPOP_REQUEST);
    memcpy(ea->arp_sha, _my_en.data(), 6);
    memcpy(ea->arp_spa, _my_ip.data(), 4);
    memset(ea->arp_tha, 0, 6);
    IPAddress want_ip = p->dst_ip_anno();
    memcpy(ea->arp_tpa, want_ip.data(), 4);

    q->set_timestamp_anno(p->timestamp_anno());
    SET_VLAN_TCI_ANNO(q, VLAN_TCI_ANNO(p));

    _arp_queries++;
    output_push(1- 1, q);
}
void
ARPQuerier_a_aarp::handle_ip(Packet *p, bool response)
{
    // delete packet if we are not configured
    if (!_my_ip) {
	p->kill();
	++_drops;
	return;
    }

    // make room for Ethernet header
    WritablePacket *q;
    if (response) {
	assert(!p->shared());
	q = p->uniqueify();
    } else if (!(q = p->push_mac_header(sizeof(click_ether)))) {
	++_drops;
	return;
    } else
	q->ether_header()->ether_type = htons(ETHERTYPE_IP);

    IPAddress dst_ip = q->dst_ip_anno();
    EtherAddress *dst_eth = reinterpret_cast<EtherAddress *>(q->ether_header()->ether_dhost);
    int r;

    // Easy case: requires only read lock
  retry_read_lock:
    r = _arpt->lookup(dst_ip, dst_eth, _poll_timeout_j);
    if (r >= 0) {
	assert(!dst_eth->is_broadcast());
	if (r > 0)
	    send_query_for(q, true);
	// ... and send packet below.
    } else if (dst_ip.addr() == 0xFFFFFFFFU || dst_ip == _my_bcast_ip) {
	memset(dst_eth, 0xff, 6);
	// ... and send packet below.
    } else if (dst_ip.is_multicast()) {
	uint8_t *dst_addr = q->ether_header()->ether_dhost;
	dst_addr[0] = 0x01;
	dst_addr[1] = 0x00;
	dst_addr[2] = 0x5E;
	uint32_t addr = ntohl(dst_ip.addr());
	dst_addr[3] = (addr >> 16) & 0x7F;
	dst_addr[4] = addr >> 8;
	dst_addr[5] = addr;
	// ... and send packet below.
    } else {
	// Zero or unknown address: do not send the packet.
	if (!dst_ip) {
	    if (!_zero_warned) {
		click_chatter("%s: would query for 0.0.0.0; missing dest IP addr annotation?", declaration().c_str());
		_zero_warned = true;
	    }
	    ++_drops;
	    q->kill();
	} else {
	    r = _arpt->append_query(dst_ip, q);
	    if (r == -EAGAIN)
		goto retry_read_lock;
	    if (r < 0)
		q->kill();
	    if (r > 0)
		send_query_for(q, false); // q is on the ARP entry's queue
	    // if r >= 0, do not q->kill() since it is stored in some ARP entry.
	}
	return;
    }

    // It's time to emit the packet with our Ethernet address as source.  (Set
    // the source address immediately before send in case the user changes the
    // source address while packets are enqueued.)
    memcpy(&q->ether_header()->ether_shost, _my_en.data(), 6);
    output_push(0, q);
}
void
ARPQuerier_a_aarp::handle_response(Packet *p)
{
    if (p->length() < sizeof(click_ether) + sizeof(click_ether_arp))
	return;

    ++_arp_responses;

    click_ether *ethh = (click_ether *) p->data();
    click_ether_arp *arph = (click_ether_arp *) (ethh + 1);
    IPAddress ipa = IPAddress(arph->arp_spa);
    EtherAddress ena = EtherAddress(arph->arp_sha);
    if (ntohs(ethh->ether_type) == ETHERTYPE_ARP
	&& ntohs(arph->ea_hdr.ar_hrd) == ARPHRD_ETHER
	&& ntohs(arph->ea_hdr.ar_pro) == ETHERTYPE_IP
	&& ntohs(arph->ea_hdr.ar_op) == ARPOP_REPLY
	&& !ena.is_group()) {
	Packet *cached_packet;
	_arpt->insert(ipa, ena, &cached_packet);

	// Send out packets in the order in which they arrived
	while (cached_packet) {
	    Packet *next = cached_packet->next();
	    handle_ip(cached_packet, true);
	    cached_packet = next;
	}
    }
}
void
ARPQuerier_a_aarp::push(int port, Packet *p)
{
    if (port == 0)
	handle_ip(p, false);
    else {
	handle_response(p);
	p->kill();
    }
}
// -*- c-basic-offset: 4 -*-
/*
 * arpresponder.{cc,hh} -- element that responds to ARP queries
 * Robert Morris
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/ethernet/arpresponder.hh"
#include <clicknet/ether.h>
#include <click/etheraddress.hh>
#include <click/ipaddress.hh>
#include <click/args.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <click/packet_anno.hh>
void *
ARPResponder_a_aarp_ur::cast(const char *n)
{
  if (void *v = ARPResponder::cast(n))
    return v;
  else if (strcmp(n, "ARPResponder@@arp_r") == 0
	  || strcmp(n, "ARPResponder") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
ARPResponder_a_aarp_ur::input_pull(int i) const
{
  if (i == 0) return ((FastClassifier_a_aarp_uc_a_aarp_uc *)input(i).element())->FastClassifier_a_aarp_uc_a_aarp_uc::pull(2);
  return input(i).pull();
}
inline void
ARPResponder_a_aarp_ur::output_push(int i, Packet *p) const
{
  if (i == 0) { ((ToDPDKDevice *)output(i).element())->ToDPDKDevice::push(0, p); return; }
  output(i).push(p);
}
inline void
ARPResponder_a_aarp_ur::output_push_checked(int i, Packet *p) const
{
  if (i < 1)
    output_push(i, p);
  else
    p->kill();
}
inline Packet *
ARPResponder_a_aarp_ur::smaction(Packet *p)
{
    const click_ether *e = (const click_ether *) p->data();
    const click_ether_arp *ea = (const click_ether_arp *) (e + 1);
    Packet *q = 0;
    if (p->length() >= sizeof(*e) + sizeof(click_ether_arp)
	&& e->ether_type == htons(ETHERTYPE_ARP)
	&& ea->ea_hdr.ar_hrd == htons(ARPHRD_ETHER)
	&& ea->ea_hdr.ar_pro == htons(ETHERTYPE_IP)
	&& ea->ea_hdr.ar_op == htons(ARPOP_REQUEST)) {
	IPAddress ipa((const unsigned char *) ea->arp_tpa);
	if (const EtherAddress *ena = lookup(ipa))
	    q = make_response(ea->arp_sha, ea->arp_spa, ena->data(), ea->arp_tpa, p);
    }
    if (q)
	p->kill();
    else
	output_push_checked ( 1,  p) ;
    return q;
}
void
ARPResponder_a_aarp_ur::push(int port, Packet *p)
{
  if (Packet *q = smaction(p))
    output_push(port, q);
}
Packet *
ARPResponder_a_aarp_ur::pull(int port)
{
  Packet *p = input_pull(port);
  return (p ? smaction(p) : 0);
}
/*
 * markipheader.{cc,hh} -- element sets IP Header annotation
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/ip/markipheader.hh"
#include <click/args.hh>
#include <clicknet/ip.h>
void *
MarkIPHeader_a_aMarkIPHeader_a13::cast(const char *n)
{
  if (void *v = MarkIPHeader::cast(n))
    return v;
  else if (strcmp(n, "MarkIPHeader@@MarkIPHeader@13") == 0
	  || strcmp(n, "MarkIPHeader") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
MarkIPHeader_a_aMarkIPHeader_a13::input_pull(int i) const
{
  if (i == 0) return ((FastClassifier_a_aarp_uc_a_aarp_uc *)input(i).element())->FastClassifier_a_aarp_uc_a_aarp_uc::pull(0);
  return input(i).pull();
}
inline void
MarkIPHeader_a_aMarkIPHeader_a13::output_push(int i, Packet *p) const
{
  if (i == 0) { ((StripToNetworkHeader_a_aStripToNetworkHeader_a14 *)output(i).element())->StripToNetworkHeader_a_aStripToNetworkHeader_a14::push(0, p); return; }
  output(i).push(p);
}
inline Packet *
MarkIPHeader_a_aMarkIPHeader_a13::smaction(Packet *p)
{
  const click_ip *ip = reinterpret_cast<const click_ip *>(p->data() + _offset);
  p->set_ip_header(ip, ip->ip_hl << 2);
  return p;
}
void
MarkIPHeader_a_aMarkIPHeader_a13::push(int port, Packet *p)
{
  if (Packet *q = smaction(p))
    output_push(port, q);
}
Packet *
MarkIPHeader_a_aMarkIPHeader_a13::pull(int port)
{
  Packet *p = input_pull(port);
  return (p ? smaction(p) : 0);
}
// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * striptonet.{cc,hh} -- element strips to network header
 * Eddie Kohler
 *
 * Copyright (c) 2001 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/standard/striptonet.hh"
#include <click/error.hh>
#include <click/glue.hh>
void *
StripToNetworkHeader_a_aStripToNetworkHeader_a14::cast(const char *n)
{
  if (void *v = StripToNetworkHeader::cast(n))
    return v;
  else if (strcmp(n, "StripToNetworkHeader@@StripToNetworkHeader@14") == 0
	  || strcmp(n, "StripToNetworkHeader") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
StripToNetworkHeader_a_aStripToNetworkHeader_a14::input_pull(int i) const
{
  if (i == 0) return ((MarkIPHeader_a_aMarkIPHeader_a13 *)input(i).element())->MarkIPHeader_a_aMarkIPHeader_a13::pull(0);
  return input(i).pull();
}
inline void
StripToNetworkHeader_a_aStripToNetworkHeader_a14::output_push(int i, Packet *p) const
{
  if (i == 0) { ((GetIPAddress_a_aGetIPAddress_a15 *)output(i).element())->GetIPAddress_a_aGetIPAddress_a15::push(0, p); return; }
  output(i).push(p);
}
inline Packet *
StripToNetworkHeader_a_aStripToNetworkHeader_a14::smaction(Packet *p)
{
    int off = p->network_header_offset();
    if (off >= 0) {
	p->pull(off);
	return p;
    } else
	return p->nonunique_push(-off);
}
void
StripToNetworkHeader_a_aStripToNetworkHeader_a14::push(int port, Packet *p)
{
  if (Packet *q = smaction(p))
    output_push(port, q);
}
Packet *
StripToNetworkHeader_a_aStripToNetworkHeader_a14::pull(int port)
{
  Packet *p = input_pull(port);
  return (p ? smaction(p) : 0);
}
/*
 * getipaddress.{cc,hh} -- element sets IP destination annotation from
 * packet header
 * Robert Morris, Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2008-2011 Meraki, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/ip/getipaddress.hh"
#include <click/args.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
void *
GetIPAddress_a_aGetIPAddress_a15::cast(const char *n)
{
  if (void *v = GetIPAddress::cast(n))
    return v;
  else if (strcmp(n, "GetIPAddress@@GetIPAddress@15") == 0
	  || strcmp(n, "GetIPAddress") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
GetIPAddress_a_aGetIPAddress_a15::input_pull(int i) const
{
  if (i == 0) return ((StripToNetworkHeader_a_aStripToNetworkHeader_a14 *)input(i).element())->StripToNetworkHeader_a_aStripToNetworkHeader_a14::pull(0);
  return input(i).pull();
}
inline void
GetIPAddress_a_aGetIPAddress_a15::output_push(int i, Packet *p) const
{
  if (i == 0) { ((FastIPClassifier_a_apc_a_apc *)output(i).element())->FastIPClassifier_a_apc_a_apc::push(0, p); return; }
  output(i).push(p);
}
inline Packet *
GetIPAddress_a_aGetIPAddress_a15::smaction(Packet *p)
{
    if (_offset >= 0)
	p->set_anno_u32(_anno, IPAddress(p->data() + _offset).addr());
    else if (_offset == offset_ip_src)
	p->set_anno_u32(_anno, p->ip_header()->ip_src.s_addr);
    else if (_offset == offset_ip_dst)
	p->set_anno_u32(_anno, p->ip_header()->ip_dst.s_addr);
    return p;
}
void
GetIPAddress_a_aGetIPAddress_a15::push(int port, Packet *p)
{
  if (Packet *q = smaction(p))
    output_push(port, q);
}
Packet *
GetIPAddress_a_aGetIPAddress_a15::pull(int port)
{
  Packet *p = input_pull(port);
  return (p ? smaction(p) : 0);
}
void *
FastIPClassifier_a_apc_a_apc::cast(const char *n)
{
  if (void *v = Element::cast(n))
    return v;
  else if (strcmp(n, "FastIPClassifier@@pc@@pc") == 0
	  || strcmp(n, "FastIPClassifier@@pc") == 0)
    return (Element *)this;
  else
    return 0;
}
inline void
FastIPClassifier_a_apc_a_apc::output_push(int i, Packet *p) const
{
  if (i == 0) { ((ICMPPingResponder_a_aICMPPingResponder_a18 *)output(i).element())->ICMPPingResponder_a_aICMPPingResponder_a18::push(0, p); return; }
  if (i == 1) { ((FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1 *)output(i).element())->FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1::push(0, p); return; }
  output(i).push(p);
}
inline void
FastIPClassifier_a_apc_a_apc::output_push_checked(int i, Packet *p) const
{
  if (i < 2)
    output_push(i, p);
  else
    p->kill();
}
void
FastIPClassifier_a_apc_a_apc::push(int, Packet *p)
{
  output_push_checked ( match(p),  p) ;
}
// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * icmppingresponder.{cc,hh} -- element constructs ICMP echo response packets
 * Robert Morris, Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2001 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/icmp/icmppingresponder.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <clicknet/ether.h>
#include <clicknet/ip.h>
#include <clicknet/icmp.h>
#include <click/packet_anno.hh>
void *
ICMPPingResponder_a_aICMPPingResponder_a18::cast(const char *n)
{
  if (void *v = ICMPPingResponder::cast(n))
    return v;
  else if (strcmp(n, "ICMPPingResponder@@ICMPPingResponder@18") == 0
	  || strcmp(n, "ICMPPingResponder") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
ICMPPingResponder_a_aICMPPingResponder_a18::input_pull(int i) const
{
  if (i == 0) return ((FastIPClassifier_a_apc_a_apc *)input(i).element())->FastIPClassifier_a_apc_a_apc::pull(0);
  return input(i).pull();
}
inline void
ICMPPingResponder_a_aICMPPingResponder_a18::output_push(int i, Packet *p) const
{
  if (i == 0) { ((ARPQuerier_a_aarp *)output(i).element())->ARPQuerier_a_aarp::push(0, p); return; }
  output(i).push(p);
}
inline Packet *
ICMPPingResponder_a_aICMPPingResponder_a18::smaction(Packet *p_in)
{
    const click_ip *iph_in = p_in->ip_header();
    const click_icmp *icmph_in = p_in->icmp_header();

    if (p_in->transport_length() < (int) sizeof(click_icmp_sequenced)
	|| iph_in->ip_p != IP_PROTO_ICMP || icmph_in->icmp_type != ICMP_ECHO) {
	if (1== 2)
	    output_push(1, p_in);
	else
	    p_in->kill();
	return 0;
    }

    WritablePacket *q = p_in->uniqueify();
    if (!q)			// out of memory
	return 0;

    // swap src and target ip addresses (checksum remains valid)
    click_ip *iph = q->ip_header();
    struct in_addr tmp_addr = iph->ip_dst;
    iph->ip_dst = iph->ip_src;
    iph->ip_src = tmp_addr;

    // clear MF, DF, etc.
    // (bug reported by David Scott Page)
    click_update_in_cksum(&iph->ip_sum, iph->ip_off, 0);
    iph->ip_off = 0;

    // set TTL to 255, update checksum
    // (bug reported by <kp13@gmx.co.uk>)
    uint16_t old_hw = ((uint16_t *)iph)[4];
    iph->ip_ttl = 255;
    uint16_t new_hw = ((uint16_t *)iph)[4];
    click_update_in_cksum(&iph->ip_sum, old_hw, new_hw);

    // set annotations
    // (dst_ip_anno bug reported by Sven Hirsch <hirschs@gmx.de>)
    q->set_dst_ip_anno(iph->ip_dst);
    q->timestamp_anno().assign_now();
    SET_PAINT_ANNO(q, 0);

    // set ICMP packet type to ICMP_ECHOREPLY and recalculate checksum
    click_icmp_echo *icmph = reinterpret_cast<click_icmp_echo *>(q->icmp_header());
    old_hw = ((uint16_t *)icmph)[0];
    icmph->icmp_type = ICMP_ECHOREPLY;
    icmph->icmp_code = 0;
    new_hw = ((uint16_t *)icmph)[0];
    click_update_in_cksum(&icmph->icmp_cksum, old_hw, new_hw);
    click_update_zero_in_cksum(&icmph->icmp_cksum, q->transport_header(), q->transport_length());

    return q;
}
void
ICMPPingResponder_a_aICMPPingResponder_a18::push(int port, Packet *p)
{
  if (Packet *q = smaction(p))
    output_push(port, q);
}
Packet *
ICMPPingResponder_a_aICMPPingResponder_a18::pull(int port)
{
  Packet *p = input_pull(port);
  return (p ? smaction(p) : 0);
}
void *
FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1::cast(const char *n)
{
  if (void *v = Element::cast(n))
    return v;
  else if (strcmp(n, "FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1") == 0
	  || strcmp(n, "FastIPClassifier@@hybrid_switch/c0/IPClassifier@1") == 0)
    return (Element *)this;
  else
    return 0;
}
inline void
FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1::output_push(int i, Packet *p) const
{
  if (i >= 0 && i <= 7) { ((Queue_a_ahybrid_uswitch_sq00 *)output(i).element())->Queue_a_ahybrid_uswitch_sq00::push(0, p); return; }
  output(i).push(p);
}
inline void
FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1::output_push_checked(int i, Packet *p) const
{
  if (i < 8)
    output_push(i, p);
  else
    p->kill();
}
void
FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1::push(int, Packet *p)
{
  output_push_checked ( match(p),  p) ;
}
// -*- c-basic-offset: 4 -*-
/*
 * fullnotequeue.{cc,hh} -- queue element that notifies on full
 * Eddie Kohler
 *
 * Copyright (c) 2004-2007 Regents of the University of California
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/standard/fullnotequeue.hh"
#include <pthread.h>
#include <clicknet/tcp.h>

void *
Queue_a_ahybrid_uswitch_sq00::cast(const char *n)
{
  if (void *v = FullNoteQueue::cast(n))
    return v;
  else if (strcmp(n, "Queue@@hybrid_switch/q00") == 0
	  || strcmp(n, "Queue") == 0)
    return (Element *)this;
  else
    return 0;
}
inline void
Queue_a_ahybrid_uswitch_sq00::output_push(int i, Packet *p) const
{
  if (i == 0) { ((RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1 *)output(i).element())->RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1::push(0, p); return; }
  output(i).push(p);
}
inline void
Queue_a_ahybrid_uswitch_sq00::output_push_checked(int i, Packet *p) const
{
  if (i < 1)
    output_push(i, p);
  else
    p->kill();
}
inline void
Queue_a_ahybrid_uswitch_sq00::push_failure(Packet *p)
{
    if (_drops == 0 && _capacity > 0)
	click_chatter("%p{element}: overflow", this);
    _drops++;
    output_push_checked ( 1,  p) ;
}
void
Queue_a_ahybrid_uswitch_sq00::push(int, Packet *p)
{
    pthread_mutex_lock(&_lock);
    // Code taken from SimpleQueue::push().
    Storage::index_type h = head(), t = tail(), nt = next_i(t);

    if (nt != h) {
        push_success(h, t, nt, p);
        enqueue_bytes += p->length();
    }
    else {
	push_failure(p);
    }
    pthread_mutex_unlock(&_lock);
}
/*
 * pullswitch.{cc,hh} -- element routes packets from one input of several
 * Eddie Kohler
 *
 * Copyright (c) 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/standard/pullswitch.hh"
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/llrpc.h>
void *
PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps::cast(const char *n)
{
  if (void *v = PullSwitch::cast(n))
    return v;
  else if (strcmp(n, "PullSwitch@@hybrid_switch/circuit_link0/ps") == 0
	  || strcmp(n, "PullSwitch") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps::input_pull(int i) const
{
  if (i >= 0 && i <= 7) return ((Queue_a_ahybrid_uswitch_sq00 *)input(i).element())->Queue_a_ahybrid_uswitch_sq00::pull(0);
  return input(i).pull();
}
Packet *
PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps::pull(int)
{
    if (_input < 0) {
	// _notifier.set_active(false, false);
	return 0;
    } else if (Packet *p = input_pull(_input)) {
	// _notifier.set_active(true, false);
	return p;
    } else {
	// if (!_signals[_input])
	//     _notifier.set_active(false, false);
	return 0;
    }
}
// -*- c-basic-offset: 4 -*-
/*
 * ratedunqueue.{cc,hh} -- element pulls as many packets as possible from
 * its input, pushes them out its output
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2010 Meraki, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "/users/mukerjee/sdrt/click/elements/standard/bwratedunqueue.hh"
void *
BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2::cast(const char *n)
{
  if (void *v = BandwidthRatedUnqueue::cast(n))
    return v;
  else if (strcmp(n, "BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2") == 0
	  || strcmp(n, "BandwidthRatedUnqueue") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2::input_pull(int i) const
{
  if (i == 0) return ((PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps *)input(i).element())->PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps::pull(0);
  return input(i).pull();
}
inline void
BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2::output_push(int i, Packet *p) const
{
  if (i == 0) { ((ARPQuerier_a_aarp *)output(i).element())->ARPQuerier_a_aarp::push(0, p); return; }
  output(i).push(p);
}
bool
BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2::run_task(Task *)
{
    bool worked = false;
    _runs++;

    if (!_active)
	return false;

    _tb.refill();

    if (_tb.contains(tb_bandwidth_thresh)) {
	if (!_can_push_signal) {
	    return false; // without rescheduling
	}
	if (Packet *p = input_pull(0)) {
	    _tb.remove(p->length());
	    _pushes++;
	    worked = true;
	    output_push(0, p);
	} else {
	    _failed_pulls++;
	    if (!_signal)
		return false;	// without rescheduling
	}
    } else {
	_timer.schedule_after(Timestamp::make_jiffies(_tb.time_until_contains(tb_bandwidth_thresh)));
	_empty_runs++;
	return false;
    }
    _task.fast_reschedule();
    if (!worked)
	_empty_runs++;
    return worked;
}
// -*- c-basic-offset: 4 -*-
/*
 * rrsched.{cc,hh} -- round robin scheduler element
 * Robert Morris, Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/error.hh>
#include "/users/mukerjee/sdrt/click/elements/standard/rrsched.hh"
void *
RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1::cast(const char *n)
{
  if (void *v = RRSched::cast(n))
    return v;
  else if (strcmp(n, "RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1") == 0
	  || strcmp(n, "RoundRobinSched") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1::input_pull(int i) const
{
  if (i >= 0 && i <= 7) return ((Queue_a_ahybrid_uswitch_sq00 *)input(i).element())->Queue_a_ahybrid_uswitch_sq00::pull(0);
  return input(i).pull();
}
Packet *
RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1::pull(int)
{
    int n = 8;
    int i = _next;
    for (int j = 0; j < n; j++) {
	Packet *p = (_signals[i] ? input_pull(i): 0);
	i++;
	if (i >= n)
	    i = 0;
	if (p) {
	    _next = i;
	    return p;
	}
    }
    return 0;
}
void *
FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1::cast(const char *n)
{
  if (void *v = Element::cast(n))
    return v;
  else if (strcmp(n, "FastIPClassifier@@in_classfy@17/IPClassifier@1@@in_classfy@17/IPClassifier@1") == 0
	  || strcmp(n, "FastIPClassifier@@in_classfy@17/IPClassifier@1") == 0)
    return (Element *)this;
  else
    return 0;
}
inline void
FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1::output_push(int i, Packet *p) const
{
  if (i >= 0 && i <= 7) { ((FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1 *)output(i).element())->FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1::push(0, p); return; }
  output(i).push(p);
}
inline void
FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1::output_push_checked(int i, Packet *p) const
{
  if (i < 8)
    output_push(i, p);
  else
    p->kill();
}
void
FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1::push(int, Packet *p)
{
  output_push_checked ( match(p),  p) ;
}
void *
BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2::cast(const char *n)
{
  if (void *v = BandwidthRatedUnqueue::cast(n))
    return v;
  else if (strcmp(n, "BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2") == 0
	  || strcmp(n, "BandwidthRatedUnqueue") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2::input_pull(int i) const
{
  if (i == 0) return ((RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1 *)input(i).element())->RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1::pull(0);
  return input(i).pull();
}
inline void
BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2::output_push(int i, Packet *p) const
{
  if (i == 0) { ((FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1 *)output(i).element())->FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1::push(0, p); return; }
  output(i).push(p);
}
bool
BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2::run_task(Task *)
{
    bool worked = false;
    _runs++;

    if (!_active)
	return false;

    _tb.refill();

    if (_tb.contains(tb_bandwidth_thresh)) {
	if (!_can_push_signal) {
	    return false; // without rescheduling
	}
	if (Packet *p = input_pull(0)) {
	    _tb.remove(p->length());
	    _pushes++;
	    worked = true;
	    output_push(0, p);
	} else {
	    _failed_pulls++;
	    if (!_signal)
		return false;	// without rescheduling
	}
    } else {
	_timer.schedule_after(Timestamp::make_jiffies(_tb.time_until_contains(tb_bandwidth_thresh)));
	_empty_runs++;
	return false;
    }
    _task.fast_reschedule();
    if (!worked)
	_empty_runs++;
    return worked;
}
void *
BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2::cast(const char *n)
{
  if (void *v = BandwidthRatedUnqueue::cast(n))
    return v;
  else if (strcmp(n, "BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2") == 0
	  || strcmp(n, "BandwidthRatedUnqueue") == 0)
    return (Element *)this;
  else
    return 0;
}
inline Packet *
BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2::input_pull(int i) const
{
  if (i == 0) return ((RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1 *)input(i).element())->RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1::pull(0);
  return input(i).pull();
}
inline void
BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2::output_push(int i, Packet *p) const
{
  if (i == 0) { ((ARPQuerier_a_aarp *)output(i).element())->ARPQuerier_a_aarp::push(0, p); return; }
  output(i).push(p);
}
bool
BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2::run_task(Task *)
{
    bool worked = false;
    _runs++;

    if (!_active)
	return false;

    _tb.refill();

    if (_tb.contains(tb_bandwidth_thresh)) {
	if (!_can_push_signal) {
	    return false; // without rescheduling
	}
	if (Packet *p = input_pull(0)) {
	    _tb.remove(p->length());
	    _pushes++;
	    worked = true;
	    output_push(0, p);
	} else {
	    _failed_pulls++;
	    if (!_signal)
		return false;	// without rescheduling
	}
    } else {
	_timer.schedule_after(Timestamp::make_jiffies(_tb.time_until_contains(tb_bandwidth_thresh)));
	_empty_runs++;
	return false;
    }
    _task.fast_reschedule();
    if (!worked)
	_empty_runs++;
    return worked;
}
#1/35           1505495575  20001 2059  600     17456     `
clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh#ifndef CLICK_clickdv_46BYcSRJf5hpUOKavaPEUc_HH
#define CLICK_clickdv_46BYcSRJf5hpUOKavaPEUc_HH
#include <click/package.hh>
#include <click/element.hh>
#include "/users/mukerjee/sdrt/click/elements/threads/staticthreadsched.hh"
#include "/users/mukerjee/sdrt/click/elements/userlevel/controlsocket.hh"
#include "/users/mukerjee/sdrt/click/elements/standard/estimate_traffic.hh"
#include "/users/mukerjee/sdrt/click/elements/standard/solstice.hh"
#include "/users/mukerjee/sdrt/click/elements/standard/run_schedule.hh"
#include "/users/mukerjee/sdrt/click/elements/standard/script.hh"
class Script_a_aScript_a6 : public Script {
 public:
   Script_a_aScript_a6() { }
   ~Script_a_aScript_a6() { }
  const char * class_name() const { return "Script@@Script@6"; }
  void * cast(const char *n);
  inline Packet * input_pull(int) const;
  inline void output_push(int, Packet *p) const;
  inline void output_push_checked(int, Packet *p) const;
  void never_devirtualize() {}
  void push(int port, Packet *p);
  Packet * pull(int);
};
#include "/users/mukerjee/sdrt/click/elements/userlevel/fromdpdkdevice.hh"
class FromDPDKDevice_a_ain : public FromDPDKDevice {
 public:
   FromDPDKDevice_a_ain() { }
   ~FromDPDKDevice_a_ain() { }
  const char * class_name() const { return "FromDPDKDevice@@in"; }
  void * cast(const char *n);
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  bool run_task(Task * t);
};
#include "/users/mukerjee/sdrt/click/elements/userlevel/todpdkdevice.hh"
#include "clickfc_LEDsdyWHqvQYWsQU4K1YFb.hh"
class FastClassifier_a_aarp_uc_a_aarp_uc : public Element {
 public:
   FastClassifier_a_aarp_uc_a_aarp_uc() { }
   ~FastClassifier_a_aarp_uc_a_aarp_uc() { }
  const char * class_name() const { return "FastClassifier@@arp_c@@arp_c"; }
  void * cast(const char *n);
  inline void output_push(int i, Packet *p) const;
  inline void output_push_checked(int i, Packet *p) const;
  void never_devirtualize() {}
  void devirtualize_all() { }
  const char * port_count() const { return "1/3"; }
  const char * processing() const { return PUSH; }
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
  int length_checked_match(const Packet *p) const;
  void push(int, Packet *p);
};
#include "/users/mukerjee/sdrt/click/elements/ethernet/arpquerier.hh"
class ARPQuerier_a_aarp : public ARPQuerier {
 public:
   ARPQuerier_a_aarp() { }
   ~ARPQuerier_a_aarp() { }
  const char * class_name() const { return "ARPQuerier@@arp"; }
  void * cast(const char *n);
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  void send_query_for(const Packet *p, bool ether_dhost_valid);
  void handle_ip(Packet *p, bool response);
  void handle_response(Packet *p);
  void push(int port, Packet *p);
};
#include "/users/mukerjee/sdrt/click/elements/ethernet/arpresponder.hh"
class ARPResponder_a_aarp_ur : public ARPResponder {
 public:
   ARPResponder_a_aarp_ur() { }
   ~ARPResponder_a_aarp_ur() { }
  const char * class_name() const { return "ARPResponder@@arp_r"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  inline void output_push(int i, Packet *p) const;
  inline void output_push_checked(int i, Packet *p) const;
  void never_devirtualize() {}
  inline Packet * smaction(Packet *p);
  void push(int port, Packet *p);
  Packet * pull(int port);
};
#include "/users/mukerjee/sdrt/click/elements/ip/markipheader.hh"
class MarkIPHeader_a_aMarkIPHeader_a13 : public MarkIPHeader {
 public:
   MarkIPHeader_a_aMarkIPHeader_a13() { }
   ~MarkIPHeader_a_aMarkIPHeader_a13() { }
  const char * class_name() const { return "MarkIPHeader@@MarkIPHeader@13"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  inline Packet * smaction(Packet *p);
  void push(int port, Packet *p);
  Packet * pull(int port);
};
#include "/users/mukerjee/sdrt/click/elements/standard/striptonet.hh"
class StripToNetworkHeader_a_aStripToNetworkHeader_a14 : public StripToNetworkHeader {
 public:
   StripToNetworkHeader_a_aStripToNetworkHeader_a14() { }
   ~StripToNetworkHeader_a_aStripToNetworkHeader_a14() { }
  const char * class_name() const { return "StripToNetworkHeader@@StripToNetworkHeader@14"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  inline Packet * smaction(Packet *p);
  void push(int port, Packet *p);
  Packet * pull(int port);
};
#include "/users/mukerjee/sdrt/click/elements/ip/getipaddress.hh"
class GetIPAddress_a_aGetIPAddress_a15 : public GetIPAddress {
 public:
   GetIPAddress_a_aGetIPAddress_a15() { }
   ~GetIPAddress_a_aGetIPAddress_a15() { }
  const char * class_name() const { return "GetIPAddress@@GetIPAddress@15"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  inline Packet * smaction(Packet *p);
  void push(int port, Packet *p);
  Packet * pull(int port);
};
class FastIPClassifier_a_apc_a_apc : public Element {
 public:
   FastIPClassifier_a_apc_a_apc() { }
   ~FastIPClassifier_a_apc_a_apc() { }
  const char * class_name() const { return "FastIPClassifier@@pc@@pc"; }
  void * cast(const char *n);
  inline void output_push(int i, Packet *p) const;
  inline void output_push_checked(int i, Packet *p) const;
  void never_devirtualize() {}
  void devirtualize_all() { }
  const char * port_count() const { return "1/2"; }
  const char * processing() const { return PUSH; }
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
  void push(int, Packet *p);
};
#include "/users/mukerjee/sdrt/click/elements/icmp/icmppingresponder.hh"
class ICMPPingResponder_a_aICMPPingResponder_a18 : public ICMPPingResponder {
 public:
   ICMPPingResponder_a_aICMPPingResponder_a18() { }
   ~ICMPPingResponder_a_aICMPPingResponder_a18() { }
  const char * class_name() const { return "ICMPPingResponder@@ICMPPingResponder@18"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  inline Packet * smaction(Packet *p_in);
  void push(int port, Packet *p);
  Packet * pull(int port);
};
class FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1 : public Element {
 public:
   FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1() { }
   ~FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1() { }
  const char * class_name() const { return "FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1"; }
  void * cast(const char *n);
  inline void output_push(int i, Packet *p) const;
  inline void output_push_checked(int i, Packet *p) const;
  void never_devirtualize() {}
  void devirtualize_all() { }
  const char * port_count() const { return "1/8"; }
  const char * processing() const { return PUSH; }
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
  void push(int, Packet *p);
};
#include "/users/mukerjee/sdrt/click/elements/standard/fullnotequeue.hh"
class Queue_a_ahybrid_uswitch_sq00 : public FullNoteQueue {
 public:
   Queue_a_ahybrid_uswitch_sq00() { }
   ~Queue_a_ahybrid_uswitch_sq00() { }
  const char * class_name() const { return "Queue@@hybrid_switch/q00"; }
  void * cast(const char *n);
  inline void output_push(int i, Packet *p) const;
  inline void output_push_checked(int i, Packet *p) const;
  void never_devirtualize() {}
  inline void push_failure(Packet *p);
  void push(int, Packet *p);
};
#include "/users/mukerjee/sdrt/click/elements/standard/pullswitch.hh"
class PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps : public PullSwitch {
 public:
   PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps() { }
   ~PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps() { }
  const char * class_name() const { return "PullSwitch@@hybrid_switch/circuit_link0/ps"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  void never_devirtualize() {}
  Packet * pull(int);
};
#include "/users/mukerjee/sdrt/click/elements/standard/bwratedunqueue.hh"
class BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2 : public BandwidthRatedUnqueue {
 public:
   BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2() { }
   ~BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2() { }
  const char * class_name() const { return "BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  bool run_task(Task *);
};
#include "/users/mukerjee/sdrt/click/elements/standard/rrsched.hh"
class RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1 : public RRSched {
 public:
   RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1() { }
   ~RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1() { }
  const char * class_name() const { return "RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  void never_devirtualize() {}
  Packet * pull(int);
};
class FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1 : public Element {
 public:
   FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1() { }
   ~FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1() { }
  const char * class_name() const { return "FastIPClassifier@@in_classfy@17/IPClassifier@1@@in_classfy@17/IPClassifier@1"; }
  void * cast(const char *n);
  inline void output_push(int i, Packet *p) const;
  inline void output_push_checked(int i, Packet *p) const;
  void never_devirtualize() {}
  void devirtualize_all() { }
  const char * port_count() const { return "1/8"; }
  const char * processing() const { return PUSH; }
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
  void push(int, Packet *p);
};
#include "/users/mukerjee/sdrt/click/elements/standard/bwratedunqueue.hh"
class BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2 : public BandwidthRatedUnqueue {
 public:
   BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2() { }
   ~BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2() { }
  const char * class_name() const { return "BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  bool run_task(Task *);
};
#include "/users/mukerjee/sdrt/click/elements/standard/bwratedunqueue.hh"
class BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2 : public BandwidthRatedUnqueue {
 public:
   BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2() { }
   ~BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2() { }
  const char * class_name() const { return "BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2"; }
  void * cast(const char *n);
  inline Packet * input_pull(int i) const;
  inline void output_push(int i, Packet *p) const;
  void never_devirtualize() {}
  bool run_task(Task *);
};
#endif
#1/27           1505495575  20001 2059  600     5826      `
elementmap-devirtualize.xml<?xml version="1.0" standalone="yes"?>
<elementmap xmlns="http://www.lcdf.org/click/xml/" package="devirtualize">
  <entry name="Script@@Script@6" cxxclass="Script_a_aScript_a6" docname="Script" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="ah/ah" flowcode="x/x" requires="userlevel Script" />
  <entry name="FromDPDKDevice@@in" cxxclass="FromDPDKDevice_a_ain" docname="FromDPDKDevice" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="h/h" flowcode="x/x" requires="userlevel FromDPDKDevice" />
  <entry name="FastClassifier@@arp_c@@arp_c" cxxclass="FastClassifier_a_aarp_uc_a_aarp_uc" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="h/h" flowcode="x/x" requires="userlevel FastClassifier@@arp_c" />
  <entry name="ARPQuerier@@arp" cxxclass="ARPQuerier_a_aarp" docname="ARPQuerier" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="h/h" flowcode="xy/x" flags="L2" requires="userlevel ARPQuerier" />
  <entry name="ARPResponder@@arp_r" cxxclass="ARPResponder_a_aarp_ur" docname="ARPResponder" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="a/ah" flowcode="x/x" requires="userlevel ARPResponder" />
  <entry name="MarkIPHeader@@MarkIPHeader@13" cxxclass="MarkIPHeader_a_aMarkIPHeader_a13" docname="MarkIPHeader" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="a/a" flowcode="x/x" requires="userlevel MarkIPHeader" />
  <entry name="StripToNetworkHeader@@StripToNetworkHeader@14" cxxclass="StripToNetworkHeader_a_aStripToNetworkHeader_a14" docname="StripToNetworkHeader" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="a/a" flowcode="x/x" requires="userlevel StripToNetworkHeader" />
  <entry name="GetIPAddress@@GetIPAddress@15" cxxclass="GetIPAddress_a_aGetIPAddress_a15" docname="GetIPAddress" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="a/a" flowcode="x/x" requires="userlevel GetIPAddress" />
  <entry name="FastIPClassifier@@pc@@pc" cxxclass="FastIPClassifier_a_apc_a_apc" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="h/h" flowcode="x/x" requires="userlevel FastIPClassifier@@pc" />
  <entry name="ICMPPingResponder@@ICMPPingResponder@18" cxxclass="ICMPPingResponder_a_aICMPPingResponder_a18" docname="ICMPPingResponder" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="a/ah" flowcode="x/x" requires="userlevel ICMPPingResponder" />
  <entry name="FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1" cxxclass="FastIPClassifier_a_ahybrid_uswitch_sc0_sIPClassifier_a1_a_ahybrid_uswitch_sc0_sIPClassifier_a1" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="h/h" flowcode="x/x" requires="userlevel FastIPClassifier@@hybrid_switch/c0/IPClassifier@1" />
  <entry name="Queue@@hybrid_switch/q00" cxxclass="Queue_a_ahybrid_uswitch_sq00" docname="Queue" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="h/lh" flowcode="x/x" requires="userlevel Queue" />
  <entry name="PullSwitch@@hybrid_switch/circuit_link0/ps" cxxclass="PullSwitch_a_ahybrid_uswitch_scircuit_ulink0_sps" docname="PullSwitch" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="l/l" flowcode="x/x" requires="userlevel PullSwitch" />
  <entry name="BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2" cxxclass="BandwidthRatedUnqueue_a_ahybrid_uswitch_scircuit_ulink0_sBandwidthRatedUnqueue_a2" docname="BandwidthRatedUnqueue" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="l/h" flowcode="x/x" requires="userlevel BandwidthRatedUnqueue" />
  <entry name="RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1" cxxclass="RoundRobinSched_a_ahybrid_uswitch_spacket_uup_ulink0_sRoundRobinSched_a1" docname="RoundRobinSched" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="l/l" flowcode="x/x" flags="S0" requires="userlevel RoundRobinSched" />
  <entry name="FastIPClassifier@@in_classfy@17/IPClassifier@1@@in_classfy@17/IPClassifier@1" cxxclass="FastIPClassifier_a_ain_uclassfy_a17_sIPClassifier_a1_a_ain_uclassfy_a17_sIPClassifier_a1" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="h/h" flowcode="x/x" requires="userlevel FastIPClassifier@@in_classfy@17/IPClassifier@1" />
  <entry name="BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2" cxxclass="BandwidthRatedUnqueue_a_ahybrid_uswitch_spacket_uup_ulink0_sBandwidthRatedUnqueue_a2" docname="BandwidthRatedUnqueue" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="l/h" flowcode="x/x" requires="userlevel BandwidthRatedUnqueue" />
  <entry name="BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2" cxxclass="BandwidthRatedUnqueue_a_ahybrid_uswitch_sps_spacket_ulink0_sBandwidthRatedUnqueue_a2" docname="BandwidthRatedUnqueue" headerfile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.hh" sourcefile="clickdv_46BYcSRJf5hpUOKavaPEUc.u.cc" processing="l/h" flowcode="x/x" requires="userlevel BandwidthRatedUnqueue" />
</elementmap>
#1/17           1505495575  20001 2059  600     1161      `
devirtualize_infoScript@@Script@6	Script
FromDPDKDevice@@in	FromDPDKDevice
FastClassifier@@arp_c@@arp_c	FastClassifier@@arp_c
ARPQuerier@@arp	ARPQuerier
ARPResponder@@arp_r	ARPResponder
MarkIPHeader@@MarkIPHeader@13	MarkIPHeader
StripToNetworkHeader@@StripToNetworkHeader@14	StripToNetworkHeader
GetIPAddress@@GetIPAddress@15	GetIPAddress
FastIPClassifier@@pc@@pc	FastIPClassifier@@pc
ICMPPingResponder@@ICMPPingResponder@18	ICMPPingResponder
FastIPClassifier@@hybrid_switch/c0/IPClassifier@1@@hybrid_switch/c0/IPClassifier@1	FastIPClassifier@@hybrid_switch/c0/IPClassifier@1
Queue@@hybrid_switch/q00	Queue
PullSwitch@@hybrid_switch/circuit_link0/ps	PullSwitch
BandwidthRatedUnqueue@@hybrid_switch/circuit_link0/BandwidthRatedUnqueue@2	BandwidthRatedUnqueue
RoundRobinSched@@hybrid_switch/packet_up_link0/RoundRobinSched@1	RoundRobinSched
FastIPClassifier@@in_classfy@17/IPClassifier@1@@in_classfy@17/IPClassifier@1	FastIPClassifier@@in_classfy@17/IPClassifier@1
BandwidthRatedUnqueue@@hybrid_switch/packet_up_link0/BandwidthRatedUnqueue@2	BandwidthRatedUnqueue
BandwidthRatedUnqueue@@hybrid_switch/ps/packet_link0/BandwidthRatedUnqueue@2	BandwidthRatedUnqueue

