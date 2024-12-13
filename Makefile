CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -D_XOPEN_SOURCE_EXTENDED
LDFLAGS = -lncursesw -lstdc++fs

SRC_DIR = src
BUILD_DIR = build
DATA_DIR = data

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TARGET = typing

.PHONY: all clean setup

all: setup $(BUILD_DIR)/$(TARGET)

setup:
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) 