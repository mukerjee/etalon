dem:
	python mkdem.py

all:
	../../bin/nsdisim -summary -sch=sched -sched=sol

dhybr:
	../../bin/nsdisim -summary -sch=sched -sched=dhybr

dcirc:
	../../bin/nsdisim -summary -sch=sched -sched=dcirc -purecirc=true

hotsp:
	../../bin/nsdisim -summary -sch=sched -sched=hotsp
