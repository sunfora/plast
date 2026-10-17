SHELL:=/usr/bin/env bash

ifeq ($(strip $(GNU_MAKE_USE_HOOKS)),1)
  load ./colorize.so
endif

CC=clang

CFLAGS=
LDFLAGS=
LDLIBS=

# specific to colorize.so
COLORIZE=
COLORIZE+=-shared
COLORIZE+=-fPIC
COLORIZE+=-g3
ifeq ($(strip $(DEBUG)),1)
  COLORIZE+=-O0 
  COLORIZE+=-fno-eliminate-unused-debug-types
else
  COLORIZE+=-O2
endif
COLORIZE+=-march=x86-64-v3
COLORIZE+=-fno-omit-frame-pointer
COLORIZE+=-fno-strict-aliasing

# shared library
LDFLAGS+=-shared
CFLAGS+=-fPIC
CFLAGS+=-g3
CFLAGS+=-O2
CFLAGS+=-march=x86-64-v3
CFLAGS+=-fno-omit-frame-pointer
CFLAGS+=-fno-strict-aliasing
CFLAGS+=-D'LULZ1 v.1'
CFLAGS+=-D"LULZ2 v.2"
CFLAGS+=-D"VERY_LULZ3 v."3
CFLAGS+="-DVERY_LULZ4 v.4"


# external dependencies
CFLAGS+=$(shell pkg-config --cflags cairo)
LDLIBS+=$(shell pkg-config --libs   cairo)

CFLAGS+=$(shell pkg-config --cflags x11)
LDLIBS+=$(shell pkg-config --libs   x11)

libplast.so: plast.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

colorize.so: colorize.c
	gcc $(COLORIZE) -o $@ $<
