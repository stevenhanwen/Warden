CXX = clang++
CXXFLAGS = -std=c++17
SRC = main.cpp
BIN = warden

.PHONY: all run clean

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
