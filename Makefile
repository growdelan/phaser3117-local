CC = /usr/bin/clang
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror -Wno-deprecated-declarations -fstack-protector-strong
LDLIBS = -lcups

all: build/rasterto3117

build/rasterto3117: src/rasterto3117.c
	mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

build/raster-fixture: tests/raster_fixture.c
	mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

test: all build/raster-fixture
	python3 tests/test_driver.py

clean:
	rm -rf build

.PHONY: all test clean

build/sandbox-probe: tests/sandbox_probe.c src/rasterto3117.c
	mkdir -p build
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

test: build/sandbox-probe

build/rasterto3117-sanitized: src/rasterto3117.c
	mkdir -p build
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer $< -o $@ $(LDLIBS)

sanitize: build/rasterto3117-sanitized build/raster-fixture build/sandbox-probe
	ASAN_OPTIONS=detect_leaks=0 PHASER_TEST_FILTER="$(CURDIR)/build/rasterto3117-sanitized" python3 tests/test_driver.py

analyze:
	$(CC) --analyze -std=c11 -Wno-deprecated-declarations src/rasterto3117.c -o /dev/null

.PHONY: sanitize analyze
