CXX      := g++
CXXFLAGS := -std=c++17 -g -Wall -I/opt/homebrew/include
LDFLAGS  := -L/opt/homebrew/lib -lraylib \
            -framework OpenGL \
            -framework Cocoa \
            -framework IOKit \
            -framework CoreVideo

TARGET   := build/main
SRCS     := $(wildcard *.cpp)
OBJS     := $(patsubst %.cpp, build/%.o, $(SRCS))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p build
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf build/
