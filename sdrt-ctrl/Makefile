CC=gcc
CFLAGS=-O3 -Wall 
LDFLAGS=-lpthread

all: server 

server: server.c
	$(CC) $(CFLAGS) $@.c $(LDFLAGS) -o $@ 

clean:
	rm *.o server

