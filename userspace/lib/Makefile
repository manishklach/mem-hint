# Patent Pending: Indian Patent Application No. 202641053160

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -I../include
AR ?= ar

all: libmemhint.a

libmemhint.a: mem_hint.o
	$(AR) rcs $@ $<

mem_hint.o: mem_hint.c ../include/mem_hint.h
	$(CC) $(CFLAGS) -c -o $@ mem_hint.c

clean:
	rm -f mem_hint.o libmemhint.a

.PHONY: all clean
