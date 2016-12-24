CFLAGS = -O3 -Wall

.PHONY: run clean

swctrl: _/swctrl/swctrl.o src/main.cc
	g++ $(CFLAGS) _/swctrl/swctrl.o src/main.cc -o swctrl

_/swctrl/swctrl.o: src/swctrl/swctrl.hh src/swctrl/swctrl.cc
	mkdir -p _/swctrl
	g++ $(CFLAGS) -c src/swctrl/swctrl.cc -o _/swctrl/swctrl.o

run: swctrl
	sudo ./swctrl

clean:
	rm -rf _ swctrl
