CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LIBS = -lSDL2 -lSDL2_ttf -lm -lpthread

SOURCES = main.cpp HistogramRenderer.cpp GaussianGenerator.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = histogram_demo

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install-deps:
	sudo apt-get update
	sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev build-essential

.PHONY: all clean install-deps
