define($IP0 10.10.1.2, $IP1 10.10.1.3, $IP2 10.10.1.4, $IP3 10.10.1.5,
       $IPSwitch 10.10.1.1,
       $MAC0 f4:52:14:15:6f:d2, $MAC1 f4:52:14:15:6d:32,
       $MAC2 0C:02:03:04:05:06, $MAC3 0D:02:03:04:05:06,
       $MACSwitch f4:52:14:15:6c:02)

c :: Classifier(12/0806 20/0002, 12/0800);
a :: ARPQuerier($IPSwitch, $MACSwitch)

cnt :: AverageCounter

Script(
	print 10, //cnt.byte_count,
	wait 1000,
	print 14,
	)

Idle ->
// FromDPDKDevice(1) ->
c

c[0] -> [1]a

c[1] -> cnt -> MarkIPHeader(14) -> StripToNetworkHeader -> GetIPAddress(16) -> a ->
// Print -> IPPrint ->
Discard
// ToDPDKDevice(1)