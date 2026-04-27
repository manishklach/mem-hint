obj-m += mem_hint_mod.o
mem_hint_mod-objs := mem_hint.o mem_hint_pmu.o \
                     mem_hint_sysfs.o mem_hint_safety.o
ccflags-y += -Wno-error

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod .*.cmd modules.order Module.symvers

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

syntax-check:
	@echo "Syntax check (no full kernel build required)..."
	gcc -fsyntax-only -D__KERNEL__ -D__GNUC__ \
	    -I/usr/src/linux-headers-$(shell uname -r)/include \
	    mem_hint.c mem_hint_pmu.c \
	    mem_hint_sysfs.c mem_hint_safety.c 2>&1 || true
	@echo "Done"

.PHONY: all clean install syntax-check
