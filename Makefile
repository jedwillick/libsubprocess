CC = gcc
CFLAGS := -std=gnu99 -MMD -Wall -pedantic -D_GNU_SOURCE -Iinclude/

TARGET = libsubprocess.so
SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o, $(SRCS))

TEST_TARGET = test/run-tests
TEST_SRCS := $(wildcard test/*.c)
TEST_OBJS := $(patsubst test/%.c,build/%.o, $(TEST_SRCS))

TEST_OPTS ?=

COVERAGE_DIR=coverage
COVERAGE_INFO=coverage.info

INSTALL_PREFIX ?= /usr/local

.PHONY: all
all: $(TARGET)

$(TARGET): CFLAGS += -O3 -fPIC
$(TARGET): LDFLAGS += -shared
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(GCOV) -o $@ $(LDLIBS) $^

build/%.o: src/%.c
	@mkdir -p build/
	$(CC) $(CFLAGS) $(GCOV) -c -o $@ $<

-include $(OBJS:.o=.d)

.PHONY: debug
debug: CFLAGS += -g -Og
debug: clean $(TARGET)

.PHONY: criterion
criterion:
	@if [ ! -d test/criterion ]; then \
		echo "Installing test framework github.com/Snaipe/Criterion ..."; \
		mkdir -p test/criterion; \
		curl -Lo- https://github.com/Snaipe/Criterion/releases/download/v2.4.1/criterion-2.4.1-linux-x86_64.tar.xz \
			| tar -x --xz --strip-components=1 -C test/criterion; \
	fi

.PHONY: test
test: $(TARGET) criterion $(TEST_TARGET)
	LD_LIBRARY_PATH=.:test/criterion/lib:$${LD_LIBRARY_PATH} $(VALGRIND) ./$(TEST_TARGET) $(TEST_OPTS)

$(TEST_TARGET): LDFLAGS += -Ltest/criterion/lib -L.
$(TEST_TARGET): LDLIBS += -lcriterion -lsubprocess
$(TEST_TARGET): CFLAGS += -g -Og -Itest/criterion/include -Wno-unused-value
$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: test/%.c
	@mkdir -p build/
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(TEST_OBJS:.o=.d)

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
	rm -f build/* $(TARGET) $(TEST_TARGET)

.PHONY: clean-coverage
clean-coverage:
	rm -rf $(COVERAGE_INFO) $(COVERAGE_DIR)

.PHONY: format
format:
	clang-format -i --verbose $$(git ls-files | grep -E "*\.[ch]")

.PHONY: install
install: $(TARGET)
	mkdir -p $(INSTALL_PREFIX)/lib
	mkdir -p $(INSTALL_PREFIX)/include
	cp $(TARGET) $(INSTALL_PREFIX)/lib
	cp -r include/subprocess $(INSTALL_PREFIX)/include

.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_PREFIX)/lib/$(TARGET)
	rm -rf $(INSTALL_PREFIX)/include/subprocess

# Put this last as the filter breaks treesitter highlights
.PHONY: memcheck
memcheck: VALGRIND = valgrind -q --leak-check=full --trace-children=yes --trace-children-skip="/usr/bin/*" --suppressions=valgrind.supp
memcheck: TEST_OPTS += --filter "!(force/sp_signal|force/sp_terminate)"
memcheck: test

