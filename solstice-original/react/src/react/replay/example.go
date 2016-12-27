package replay

var Example = `
n=4
linkbw=1250
packbw=125
chan=send,recv,drop,sched

t=0 // dusk,new-day,new-week
send: - 3 3 3, 3 - 3 2, 4 3 - 3, 7 3 2 -
recv: - 3 3 2, 3 - 3 2, 4 3 - 2, 7 3 2 -
drop: - - - 1, - - - -, - - - 1, - - - -
sched: - - - -, - - - -, - - - -, - - - -


t=1 // dawn
send: - 3 3 3, 3 - 3 2, 4 3 - 3, 7 3 2 -
recv: - 3 3 2, 3 - 3 2, 4 3 - 2, 7 3 2 -
drop: - - - 1, - - - -, - - - 1, - - - -

`
