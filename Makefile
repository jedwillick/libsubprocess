CC = gcc
CFLAGS = -MMD -Wall -pedantic -std=gnu99

TARGET = subprocess
SRCS := $(shell find src -name "*.c")
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

$(OBJS): CFLAGS += -O3
$(OBJS): $(SRCS)

-include $(DEPS)

.PHONY: debug
debug: CFLAGS += -g -Og
debug: clean $(TARGET)

.PHONY: clean
clean:
	rm -f $(OBJS)
