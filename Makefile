CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Isrc -I/opt/homebrew/opt/googletest/include

SRC_DIR := src
BUILD_DIR := build
BIN := warden
TEST_BIN := test_runner

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
LIB_OBJS := $(filter-out $(BUILD_DIR)/display.o, $(OBJECTS))  # everything except display.o
DEPS := $(OBJECTS:.o=.d)

TEST_SRCS := $(wildcard tests/*.cpp)
TEST_OBJS := $(patsubst tests/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS))

.PHONY: all run clean rebuild test

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ -lncurses

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/tests/%.o: tests/%.cpp | $(BUILD_DIR)/tests
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
	
$(TEST_BIN): $(TEST_OBJS) $(LIB_OBJS)
	$(CXX) $^ -o $@ -L/opt/homebrew/opt/googletest/lib -lgtest -lgtest_main -pthread

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/tests:
	mkdir -p $@

run: $(BIN)
	./$(BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN) $(TEST_BIN)

rebuild: clean all

-include $(DEPS)
