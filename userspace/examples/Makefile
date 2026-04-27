CC     = gcc
CFLAGS = -O2 -Wall -I../include
LDFLAGS = -L../lib -lmem_hint

all: basic_hint phase_monitor

basic_hint: basic_hint.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

phase_monitor: phase_monitor.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f basic_hint phase_monitor

.PHONY: all clean
