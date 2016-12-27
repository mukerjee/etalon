MKDEM=../../bin/mkdem
CONF=../../bin/config
SIM=../../bin/tbsim
RUN=../../bin/tbrun2
WEBSIM=../../bin/websim
WEBDIFF=../../bin/webdiff
DIFFCHAN=recv

.PHONY: all sim run report clean websim webdiff

all: dem

dem: 
	$(MKDEM) -dem=demand

sim:
	$(SIM) -dem=demand -sch=sim.sch -rep=sim.rep

run:
	$(RUN) -dem=demand -rep=run.rep

report:
	@ echo "todo"

clean:
	-rm -rf demand *.sch *.rep

websim:
	- $(WEBSIM) -dem=demand -web=../../www/websim

conf:
	$(CONF)

webdiff:
	- $(WEBDIFF) -rep1=sim.rep -rep2=run.rep -web=../../www/webdiff -chan=$(DIFFCHAN)

cali: all webdiff

calir: run webdiff

thput:
	../../bin/thput -rep=sim.rep
	../../bin/thput -rep=run.rep
