SHELL:=/usr/bin/env bash
load ./colorize.so

CC=clang

CFLAGS=
LDFLAGS=
LDLIBS=

# shared library
LDFLAGS+=-shared
CFLAGS+=-fPIC
CFLAGS+=-fno-omit-frame-pointer
CFLAGS+=-fno-strict-aliasing


# external dependencies
CFLAGS+=$(shell pkg-config --cflags cairo)
LDLIBS+=$(shell pkg-config --libs   cairo)

CFLAGS+=$(shell pkg-config --cflags x11)
LDLIBS+=$(shell pkg-config --libs   x11)

libplast.so: plast.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)
