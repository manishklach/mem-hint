CC     = gcc
CFLAGS = -O2 -Wall -Wextra -I../include
AR     = ar

all: libmem_hint.a

mem_hint.o: mem_hint.c
	$(CC) $(CFLAGS) -c -o $@ $<

libmem_hint.a: mem_hint.o
	$(AR) rcs $@ $<

clean:
	rm -f *.o *.a

.PHONY: all clean
