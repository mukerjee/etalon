.PHONY: all clean fmt tags run lib

CFLAGS=-Wall -O3 # -std=c89
CC=gcc

all: sols lib

_/%.o: %.c *.h
	@mkdir -p _
	$(CC) $(CFLAGS) $< -c -o $@

sols: _/main.o _/sols.o
	$(CC) $(CFLAGS) $^ -lrt -o $@

clean:
	rm -rf _

fmt:
	astyle *.h *.c -nQ

tags:
	ctags -h=.c.h -R

run: sols
	./sols

lib: libcsol.a

libcsol.a: _/sols.o
	@-rm -f $@
	ar -cvq $@ $^
