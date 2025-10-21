# Makefile for Secret Hitler MCTS Bot

CXX       := g++
CXXFLAGS  := -std=c++17 -O3 -Iinclude -Wall -Wextra

SRC_DIR   := src
SRCS      := $(wildcard $(SRC_DIR)/*.cpp) main.cpp
OBJS      := $(SRCS:.cpp=.o)

TARGET    := secret_hitler_bot

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
