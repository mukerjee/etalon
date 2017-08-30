define($DEV enp8s0d1, $DADDR 10.10.1.3, $GW $DEV:gw, $METHOD PCAP)

Script(wait 2,
       print ping.summary,
       stop)

// FromDevice($DEV, SNIFFER false, METHOD $METHOD)
FromDPDKDevice(1)
	-> c :: Classifier(12/0800, 12/0806 20/0002)
	-> CheckIPHeader(14)
	-> ip :: IPClassifier(icmp echo-reply)
	-> ping :: ICMPPingSource($DEV, $DADDR, INTERVAL 0.1,
				  LIMIT 10, VERBOSE true)
	-> SetIPAddress($GW)
	-> arpq :: ARPQuerier($DEV)
	-> ToDPDKDevice(1)
	// -> q :: Queue -> ToDevice($DEV);

c[1]	-> [1]arpq;
