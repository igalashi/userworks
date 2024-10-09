#
#
#

CC = gcc
CFLAGS = -Wall -g -O
CXX = g++
CXXFLAGS = -Wall -g -O --std=c++17

LDFLAGS = -lasound -lm

ROOTINCDIR = $(shell $(ROOTSYS)/bin/root-config --incdir)
ROOTLIBDIR = $(shell $(ROOTSYS)/bin/root-config --libdir)
#ROOTLIBS = $(shell $(ROOTSYS)/bin/root-config --libs) \
#	-lNew -Xlinker -rpath $(ROOTSYS)/lib
ROOTLIBS = $(shell $(ROOTSYS)/bin/root-config --libs) \
	-lNew -Xlinker
ROOTGLIBS = $(shell $(ROOTSYS)/bin/root-config --glibs)

EXECS = checkfilter chkfile chkraw

all: $(EXECS)


checkfilter: checkfilter.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		-I$(ROOTINCDIR) \
		$< \
		# -L$(ROOTLIBDIR) $(ROOTLIBS) $(ROOTGLIBS) \
		# $(LDFLAGS) \
		# -lThread

chkfile: chkfile.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		-I$(ROOTINCDIR) \
		$<

chkraw: chkraw.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		-I$(ROOTINCDIR) \
		$<

clean:
	rm $(EXECS)
