CC = gcc
CFLAGS := -std=gnu99 -MMD -Wall -pedantic -Iinclude/

TARGET = libsubprocess.so
SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o, $(SRCS))

.PHONY: all
all: $(TARGET)

$(TARGET): CFLAGS += -O3 -fPIC
$(TARGET): LDFLAGS += -shared
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(GCOV) -o $@ $(LDLIBS) $^

build/%.o: src/%.c
	mkdir -p build/
	$(CC) $(CFLAGS) $(GCOV) -c -o $@ $<

-include $(OBJS:.o=.d)

.PHONY: debug
debug: CFLAGS += -g -Og
debug: clean $(TARGET)

.PHONY: clean
clean:
	rm -f $(OBJS)
