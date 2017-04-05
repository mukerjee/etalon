define($IP0 1.1.1.1, $IP1 1.1.1.2, $IP2 1.1.1.3, $IP3 1.1.1.4)

ControlSocket("TCP", 1239)

// aa :: {InfiniteSource -> IPEncap(255, $IP0, $IP0) -> output}
// ab :: {InfiniteSource -> IPEncap(255, $IP0, $IP1) -> output}
// ac :: {InfiniteSource -> IPEncap(255, $IP0, $IP2) -> output}
// ad :: {InfiniteSource -> IPEncap(255, $IP0, $IP3) -> output}

// ba :: {InfiniteSource -> IPEncap(255, $IP1, $IP0) -> output}
// bb :: {InfiniteSource -> IPEncap(255, $IP1, $IP1) -> output}
// bc :: {InfiniteSource -> IPEncap(255, $IP1, $IP2) -> output}
// bd :: {InfiniteSource -> IPEncap(255, $IP1, $IP3) -> output}

// ca :: {InfiniteSource -> IPEncap(255, $IP2, $IP0) -> output}
// cb :: {InfiniteSource -> IPEncap(255, $IP2, $IP1) -> output}
// cc :: {InfiniteSource -> IPEncap(255, $IP2, $IP2) -> output}
// cd :: {InfiniteSource -> IPEncap(255, $IP2, $IP3) -> output}

// da :: {InfiniteSource -> IPEncap(255, $IP3, $IP0) -> output}
// db :: {InfiniteSource -> IPEncap(255, $IP3, $IP1) -> output}
// dc :: {InfiniteSource -> IPEncap(255, $IP3, $IP2) -> output}
// dd :: {InfiniteSource -> IPEncap(255, $IP3, $IP3) -> output}

// aa, ab, ac, ad -> a :: Null
// ba, bb, bc, bd -> b :: Null
// ca, cb, cc, cd -> c :: Null
// da, db, dc, dd -> d :: Null

// elementclass Checker {
//     // c :: IPClassifier(src host $IP0, src host $IP1, src host $IP2, src host $IP3)
//     // c0, c1, c2, c3 :: Counter
//     c0 :: Counter
//     // input -> c => c0, c1, c2, c3 -> Discard
//     input -> c0 -> Discard
// }

// out0 :: Checker
// out1 :: Checker
// out2 :: Checker
// out3 :: Checker

// a, b, c, d => out0, out1, out2, out3

InfiniteSource(LENGTH 1) -> Queue -> Unqueue -> c0 :: Counter -> Queue -> Discard
