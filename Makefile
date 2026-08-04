CXX      := g++
CXXFLAGS := -std=c++17 -g -Wall -I/opt/homebrew/include -Isrc
LDFLAGS  := -L/opt/homebrew/lib -lraylib \
            -framework OpenGL \
            -framework Cocoa \
            -framework IOKit \
            -framework CoreVideo

TARGET   := build/main
SRCS     := $(wildcard src/*.cpp)
OBJS     := $(patsubst src/%.cpp, build/%.o, $(SRCS))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p build
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf build/
