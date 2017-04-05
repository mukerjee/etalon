define($IP0 1.1.1.1, $IP1 1.1.1.2, $IP2 1.1.1.3, $IP3 1.1.1.4,
       $MAC0 0A:02:03:04:05:06, $MAC1 0B:02:03:04:05:06,
       $MAC2 0C:02:03:04:05:06, $MAC3 0D:02:03:04:05:06,
       $MACSwitch 0F:02:03:04:05:06)

ControlSocket("TCP", 1239)

in :: FromDPDKDevice(...)
out :: ToDPDKDevice(...)

elementclass Checker { $mac |
    c :: IPClassifier(src host $IP0, src host $IP1, src host $IP2, src host $IP3)
    c0, c1, c2, c3 :: AverageCounter
    input -> Unqueue -> c => c0, c1, c2, c3
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

    q00, q01, q02, q03 :: Queue(CAPACITY 1000)
    q10, q11, q12, q13 :: Queue(CAPACITY 1000)
    q20, q21, q22, q23 :: Queue(CAPACITY 1000)
    q30, q31, q32, q33 :: Queue(CAPACITY 1000)

    wrr0 :: WeightedRoundRobinSched(WEIGHTS "1,41,1,1")
    wrr1 :: WeightedRoundRobinSched(WEIGHTS "1,1,41,1")
    wrr2 :: WeightedRoundRobinSched(WEIGHTS "1,1,1,41")
    wrr3 :: WeightedRoundRobinSched(WEIGHTS "41,1,1,1")

    input[0] -> c0 => q00, q01, q02, q03
    input[1] -> c1 => q10, q11, q12, q13
    input[2] -> c2 => q20, q21, q22, q23
    input[3] -> c3 => q30, q31, q32, q33

    q00, q10, q20, q30 => wrr0 -> [0]output
    q01, q11, q21, q31 => wrr1 -> [1]output
    q02, q12, q22, q32 => wrr2 -> [2]output
    q03, q13, q23, q33 => wrr3 -> [3]output
}

in -> MarkIPHeader(14)
   -> IPClassifier(src host $IP0, src host $IP1, src host $IP2, src host $IP3)[0, 1, 2, 3]
   => hybrid_switch => out0, out1, out2, out3 -> out
