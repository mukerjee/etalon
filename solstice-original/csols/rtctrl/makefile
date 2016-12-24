.PHONY: fmt run tags

CFLAGS=-O3 -Wall
CPP=g++

rtctrl: _/sols.o _/main.o _/swctrl.o
	$(CPP) $(CFLAGS) _/sols.o _/swctrl.o _/main.o -o rtctrl -lsnf -L.

_/sols.o: sols/sols.c sols/sols.h
	@mkdir -p _
	$(CPP) $(CFLAGS) sols/sols.c -c -o _/sols.o 

_/swctrl.o: swctrl/swctrl.cc swctrl/swctrl.hh
	@mkdir -p _
	$(CPP) $(CFLAGS) swctrl/swctrl.cc -c -o _/swctrl.o

_/main.o: _/sols.o sols/sols.h *.h main.cpp
	@mkdir -p _
	$(CPP) $(CFLAGS) main.cpp -c -o _/main.o

fmt:
	astyle *.h *.cpp -nQ -A2

run: rtctrl
	LD_LIBRARY_PATH=`pwd` ./rtctrl
tags:
	ctags -h=.cc.h *
