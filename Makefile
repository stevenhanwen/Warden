CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic

SRC_DIR := src
BUILD_DIR := build
BIN := warden

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)

.PHONY: all run clean rebuild

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN)

rebuild: clean all

-include $(DEPS)
