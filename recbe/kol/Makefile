#
#
#
CC = gcc
CFLAGS = -Wall -g -O
CLIBS = 

CXX = g++
CXXFLAGS = -Wall -g -O
CXXLIBS = -L. -lkol

INCLUDES =

all: libkol.a

libkol.a: koltcp.o kolsocket.o koluri.o kolthread.o
	ar r $@ $^

ex1: ex1.cpp libkol.a
	$(CXX) $(CXXFLAGS) -o $@ $< $(CXXLIBS)
	
clean:
	rm -f *.o *.a

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
