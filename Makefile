NAME := pllzdMLMC

# Specify the C++ compiler
CXX ?= g++

# Compiler flags
CXXFLAGS = -std=c++20 -O3 -lboost_iostreams -lboost_system -lboost_filesystem -fopenmp
CXXFLAGS += -Wconversion -Wall -Werror -Wextra -pedantic -g -I/opt/homebrew/opt/curl/include

# Linker flags and libraries
LDFLAGS = -L/opt/homebrew/opt/curl/lib
LIBS = -lcurl

# Define the source files used in the project
SRCS = $(wildcard *.cpp)
SRCS += stockast.cpp
OBJS = $(SRCS:.cpp=.o)

# Specify the target executable
EXE = mcml

# Default rule to build the executable
mcml: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $^ -o $@ $(LIBS) $(LDFLAGS)
	
mcml.o: mcml.cpp

.PHONY: clean

# Rule to compile C++ source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove object files and the executable
clean:
	rm -f $(OBJS) $(EXE)
	rm -f *.o mcml
