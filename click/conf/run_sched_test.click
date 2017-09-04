elementclass Checker {
    c0 :: Counter
    input -> c0 -> output
}

out0 :: Checker()

scripte :: Script_New(
       write r.setSchedule 2 180 1/2/3/0 20 4/4/4/4,
       // print out0/c0.count,
       // wait 1,
       // loop
       )

StaticThreadSched(r 2, scripte 0)
r :: RunSchedule(1)



InfiniteSource(LENGTH 9000) -> out0 -> Discard

