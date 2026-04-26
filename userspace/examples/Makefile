# Patent Pending: Indian Patent Application No. 202641053160

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -I../include

all: basic_hint phase_monitor

basic_hint: basic_hint.c ../include/mem_hint.h
	$(CC) $(CFLAGS) -o $@ basic_hint.c

phase_monitor: phase_monitor.c ../lib/mem_hint.c ../include/mem_hint.h
	$(CC) $(CFLAGS) -o $@ phase_monitor.c ../lib/mem_hint.c

clean:
	rm -f basic_hint phase_monitor

.PHONY: all clean
