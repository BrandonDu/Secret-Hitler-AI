LLVM_PATH := /opt/homebrew/opt/llvm
TORCH_DIR := /Users/brandondu/libtorch

CXX       := $(LLVM_PATH)/bin/clang++
CXXFLAGS  := -std=c++17 -O3 -Iinclude -Wall -Wextra -march=native -DNDEBUG -pthread
CXXFLAGS  += -fopenmp -I$(LLVM_PATH)/include -I$(LLVM_PATH)/include/c++/v1
CXXFLAGS  += -I$(TORCH_DIR)/include -I$(TORCH_DIR)/include/torch/csrc/api/include

LDFLAGS   := -L$(LLVM_PATH)/lib -fopenmp
LDFLAGS   += -L$(TORCH_DIR)/lib -Wl,-rpath,$(TORCH_DIR)/lib -ltorch -ltorch_cpu -lc10

SRC_DIR   := src
SRCS      := $(wildcard $(SRC_DIR)/*.cpp) main.cpp
OBJS      := $(SRCS:.cpp=.o)
TARGET    := secret_hitler_bot

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
