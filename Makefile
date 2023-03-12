CC = gcc
CFLAGS := -std=gnu99 -MMD -Wall -pedantic -Iinclude/
DEBUG_CFLAGS = -g -Og -DDEBUG

TARGET_STATIC = libsubprocess.a
TARGET_SHARED = libsubprocess.so
SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o, $(SRCS))

TEST_TARGET = test/run-tests
TEST_SRCS := $(wildcard test/*_test.c)
TEST_OBJS := $(patsubst test/%.c,build/%.o, $(TEST_SRCS))
TEST_OPTS ?=

VALGRIND = valgrind -s --leak-check=full --show-leak-kinds=all --trace-children=yes --trace-children-skip="/usr/bin/*"
MEMCHECK_TARGET = test/memcheck
TARGETS := $(TARGET_SHARED) $(TARGET_STATIC) $(TEST_TARGET) $(MEMCHECK_TARGET)

COVERAGE_DIR=coverage
COVERAGE_INFO=coverage.info

INSTALL_PREFIX ?= /usr/local

CREATE_BUILD_DIRS := $(shell mkdir -p build/)

.PHONY: all
all: $(TARGET_SHARED) $(TARGET_STATIC)

$(TARGET_SHARED): LDFLAGS += -shared
$(TARGET_SHARED): $(OBJS)
	$(CC) $(LDFLAGS) $(GCOV) -o $@ $(LDLIBS) $^


$(TARGET_STATIC): $(OBJS)
	ar rcs $@ $^

build/%.o: CFLAGS += -O3 -fPIC
build/%.o: src/%.c
	$(CC) $(CFLAGS) $(GCOV) -c -o $@ $<

-include $(OBJS:.o=.d)

.PHONY: debug
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: clean all

.PHONY: criterion
criterion:
	@if [ ! -d test/criterion ]; then \
		echo "Installing test framework github.com/Snaipe/Criterion ..."; \
		mkdir -p test/criterion; \
		curl -Lo- https://github.com/Snaipe/Criterion/releases/download/v2.4.1/criterion-2.4.1-linux-x86_64.tar.xz \
			| tar -x --xz --strip-components=1 -C test/criterion; \
	fi

.PHONY: test
test: $(TARGET_SHARED) criterion $(TEST_TARGET)
	LD_LIBRARY_PATH=.:test/criterion/lib:$${LD_LIBRARY_PATH} ./$(TEST_TARGET) $(TEST_OPTS)

$(TEST_TARGET): LDFLAGS += -Ltest/criterion/lib -L.
$(TEST_TARGET): LDLIBS += -lcriterion -lsubprocess
$(TEST_TARGET): CFLAGS += -Itest/criterion/include -Wno-unused-value $(DEBUG_CFLAGS)
$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: test/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(TEST_OBJS:.o=.d)

$(MEMCHECK_TARGET): CFLAGS += $(DEBUG_CFLAGS)
$(MEMCHECK_TARGET): LDLIBS += -lsubprocess
$(MEMCHECK_TARGET): LDFLAGS += -L.
$(MEMCHECK_TARGET): test/memcheck.c

.PHONY: memcheck
memcheck: $(TARGET_SHARED) $(MEMCHECK_TARGET)
	LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH} $(VALGRIND) ./test/memcheck

.PHONY: coverage
coverage: GCOV = --coverage
coverage: TEST_OPTS += --always-succeed
coverage: test
	lcov --capture --directory . --output-file $(COVERAGE_INFO)
	lcov --remove $(COVERAGE_INFO) '/usr/*' --output-file $(COVERAGE_INFO)
	lcov --list $(COVERAGE_INFO)

.PHONY: coverage-html
coverage-html: coverage
	genhtml $(COVERAGE_INFO) --output-directory $(COVERAGE_DIR)
	python3 -m http.server --bind localhost --directory $(COVERAGE_DIR)

.PHONY: clean
clean:
	rm -f $(TARGETS)
	rm -rf build/*

.PHONY: clean-coverage
clean-coverage:
	rm -rf $(COVERAGE_INFO) $(COVERAGE_DIR)

.PHONY: format
format:
	clang-format -i --verbose $$(git ls-files | grep -E "*\.[ch]")

.PHONY: install
install: $(TARGET_STATIC) $(TARGET_SHARED)
	mkdir -p $(INSTALL_PREFIX)/lib
	mkdir -p $(INSTALL_PREFIX)/include
	cp $(TARGET_SHARED) $(INSTALL_PREFIX)/lib
	cp $(TARGET_STATIC) $(INSTALL_PREFIX)/lib
	cp -r include/subprocess $(INSTALL_PREFIX)/include

.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_PREFIX)/lib/$(TARGET_SHARED)
	rm -f $(INSTALL_PREFIX)/lib/$(TARGET_STATIC)
	rm -rf $(INSTALL_PREFIX)/include/subprocess

.PHONY: bear
bear: clean
	bear -- $(MAKE) all


.PHONY: doc
doc:
	@mkdir -p build/
	cd doc && doxygen

.PHONY: doc-local
doc-local: doc
	python3 -m http.server --bind localhost --directory build/doc/html/

