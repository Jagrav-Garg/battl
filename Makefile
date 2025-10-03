CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Iinclude
SRC = src/engine.cpp src/strat_a.cpp src/strat_b.cpp
TARGET = tank_sim

all: $(TARGET)

$(TARGET): $(SRC) include/defs.hpp include/scor.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
