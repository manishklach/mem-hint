obj-m += mem_hint.o

mem_hint-objs := mem_hint_main.o \
		 mem_hint_pmu.o \
		 mem_hint_sysfs.o \
		 mem_hint_safety.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

check-headers:
	@if [ ! -d "$(KDIR)" ]; then \
	  echo "WARN: $(KDIR) not found, trying generic headers"; \
	  KDIR=/usr/src/linux-headers-$$(ls /usr/src | \
	        grep linux-headers | grep -v generic | head -1 | \
	        sed 's/linux-headers-//'); \
	fi

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod .*.cmd modules.order Module.symvers

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

syntax-check:
	@echo "Running syntax check (no kernel build required)..."
	gcc -fsyntax-only -D__KERNEL__ -D__GNUC__ \
	    -I/usr/src/linux-headers-$(shell uname -r)/include \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include \
	    mem_hint_main.c mem_hint_pmu.c mem_hint_sysfs.c \
	    mem_hint_safety.c 2>&1 || true
	@echo "Syntax check complete"

.PHONY: all clean install syntax-check check-headers
