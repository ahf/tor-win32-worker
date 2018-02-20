CC     = gcc
WIN_CC = i686-w64-mingw32-gcc
CFLAGS = -Wall -O2 -Werror

LIBEVENT_DIR     = ../tor-win32/prefix
LIBEVENT_INCLUDE = $(LIBEVENT_DIR)/include
LIBEVENT_LIB     = $(LIBEVENT_DIR)/lib

all: test.exe test-child.exe test test-child

test.exe: test.c
	$(WIN_CC) $(CFLAGS) -I$(LIBEVENT_INCLUDE) -L$(LIBEVENT_LIB) $< $(LIBEVENT_LIB)/libevent.a -lws2_32 -lgdi32 -lwsock32 -o $@

test-child.exe: test-child.c
	$(WIN_CC) $(CFLAGS) -o $@ $<

test: test.c
	$(CC) $(CFLAGS) -levent -o $@ $<

test-child: test-child.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf test test-child *.exe || true

run-win: test.exe
	wine ./test.exe

run-unix: test
	./test

.phony: clean run-win run-unix
