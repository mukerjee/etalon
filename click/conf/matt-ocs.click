define($IP0 1.1.1.1, $IP1 1.1.1.2, $IP2 1.1.1.3, $IP3 1.1.1.4)

ControlSocket("TCP", 1239)

aa :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP0, $IP0) -> output}
ab :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP0, $IP1) -> output}
ac :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP0, $IP2) -> output}
ad :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP0, $IP3) -> output}

ba :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP1, $IP0) -> output}
bb :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP1, $IP1) -> output}
bc :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP1, $IP2) -> output}
bd :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP1, $IP3) -> output}

ca :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP2, $IP0) -> output}
cb :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP2, $IP1) -> output}
cc :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP2, $IP2) -> output}
cd :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP2, $IP3) -> output}

da :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP3, $IP0) -> output}
db :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP3, $IP1) -> output}
dc :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP3, $IP2) -> output}
dd :: {InfiniteSource(LENGTH 9000) -> IPEncap(255, $IP3, $IP3) -> output}

aa, ab, ac, ad -> a :: Null
ba, bb, bc, bd -> b :: Null
ca, cb, cc, cd -> c :: Null
da, db, dc, dd -> d :: Null

elementclass Checker {
    c :: IPClassifier(src host $IP0, src host $IP1, src host $IP2, src host $IP3)
    c0, c1, c2, c3 :: AverageCounter
    input -> Unqueue -> c => c0, c1, c2, c3 -> Queue -> Discard
}

out0 :: Checker
out1 :: Checker
out2 :: Checker
out3 :: Checker

hybrid_switch :: {
    c0 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c1 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c2 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)
    c3 :: IPClassifier(dst host $IP0, dst host $IP1, dst host $IP2, dst host $IP3)

    q00, q01, q02, q03 :: Queue(CAPACITY 1000)
    q10, q11, q12, q13 :: Queue(CAPACITY 1000)
    q20, q21, q22, q23 :: Queue(CAPACITY 1000)
    q30, q31, q32, q33 :: Queue(CAPACITY 1000)

    input[0] -> c0 => q00, q01, q02, q03
    input[1] -> c1 => q10, q11, q12, q13
    input[2] -> c2 => q20, q21, q22, q23
    input[3] -> c3 => q30, q31, q32, q33

    wrr0 :: WeightedRoundRobinSched(WEIGHTS "1,41,1,1")
    wrr1 :: WeightedRoundRobinSched(WEIGHTS "1,1,41,1")
    wrr2 :: WeightedRoundRobinSched(WEIGHTS "1,1,1,41")
    wrr3 :: WeightedRoundRobinSched(WEIGHTS "41,1,1,1")

    q00, q10, q20, q30 => wrr0 -> [0]output
    q01, q11, q21, q31 => wrr1 -> [1]output
    q02, q12, q22, q32 => wrr2 -> [2]output
    q03, q13, q23, q33 => wrr3 -> [3]output

    // hs :: HybridSwitch(WEIGHTS " 1, 41,  1,  1;
    //                              1,  1, 41,  1;
    //                              1,  1,  1, 41;
    //                             41,  1,  1,  1;")
    // q00, q10, q20, q30,
    // q01, q11, q21, q31,
    // q02, q12, q22, q32,
    // q03, q13, q23, q33 => hs => [0]output, [1]output, [2]output, [3]output
}

a, b, c, d => hybrid_switch => out0, out1, out2, out3
