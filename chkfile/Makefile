#
#
#

CC = gcc
CFLAGS = -Wall -g -O
CXX = g++
CXXFLAGS = -Wall -g -O --std=c++17

#LDFLAGS = -lasound -lm
LDFLAGS = -lm -lstdc++fs

ROOTINCDIR = $(shell $(ROOTSYS)/bin/root-config --incdir)
ROOTLIBDIR = $(shell $(ROOTSYS)/bin/root-config --libdir)
#ROOTLIBS = $(shell $(ROOTSYS)/bin/root-config --libs) \
#	-lNew -Xlinker -rpath $(ROOTSYS)/lib
ROOTLIBS = $(shell $(ROOTSYS)/bin/root-config --libs) \
	-lNew -Xlinker
ROOTGLIBS = $(shell $(ROOTSYS)/bin/root-config --glibs)

EXECS = checkfilter chkfile chkraw chkcydet

all: $(EXECS)


checkfilter: checkfilter.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		$< \
		$(LDFLAGS)

chkfile: chkfile.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		$< \
		$(LDFLAGS)

chkraw: chkraw.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		$< \
		$(LDFLAGS)

chkcydet: chkcydet.cxx UnpackRecbe.h
	$(CXX) $(CXXFLAGS) -o $@ \
		$< \
		$(LDFLAGS)

clean:
	rm $(EXECS)
