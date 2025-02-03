#
#
#

CC = gcc
CFLAGS = -Wall -g -O
CXX = g++
CXXFLAGS = -Wall -g -O --std=c++17
LDFLAGS = -lm -lstdc++fs

UHBOOK = -I../../uhbook
NESTDAQ_INC = -I$(NESTDAQ)/include
NESTDAQ_LIB = -L$(NESTDAQ)/lib -L$(NESTDAQ)/lib64 -lhiredis -lredis++
NESTDAQ_RPATH = -Wl,-rpath,$(NESTDAQ)/lib:$(NESTDAQ)/lib64


EXECS = redisperf

all: $(EXECS)


redisperf: redisperf.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		$(NESTDAQ_INC) \
		$(UHBOOK) \
		$< \
		$(NESTDAQ_LIB) \
		$(NESTDAQ_RPATH) \
		$(LDFLAGS)

clean:
	rm $(EXECS)
