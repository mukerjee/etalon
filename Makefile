CC = g++
CFLAGS = -Wall -O3 -Wno-unused-variable -Wno-unused-result 
LDFLAGS = -lrt -lpthread -lnfnetlink -lnetfilter_queue
NFQUEUE_TEST = nfqueue_test.o sols.o
all: nfqueue_test
nfqueue_test: $(NFQUEUE_TEST)
	$(CC) $(CFLAGS)  $(NFQUEUE_TEST) -o nfqueue_test $(LDFLAGS)

clean:
	rm -f *.o
	rm -f nfqueue_test

