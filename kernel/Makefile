obj-m += mem_hint.o

mem_hint-objs := mem_hint_main.o \
		 mem_hint_pmu.o \
		 mem_hint_sysfs.o \
		 mem_hint_safety.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

.PHONY: all clean install
