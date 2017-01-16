CXX=g++
CXXFLAGS = -Wall -O3 -std=gnu++11 -Wno-unused-variable -Wno-unused-result
LDFLAGS = -lrt -lpthread -lnfnetlink -lnetfilter_queue
BIN=switch

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%.o)

all: $(OBJ)
	$(CXX) $(LDFLAGS) -o $(BIN) $^

%.o: %.c
	$(CXX) $@ -c $<

clean:
	rm -f *.o
	rm $(BIN)
