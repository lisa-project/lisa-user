obj-$(CONFIG_SWITCH) += switch.o
switch-objs := module.o fdb.o

EXTRA_CFLAGS += -DDEBUG

# KDIR := /lib/modules/$(KVER)/build
KDIR := ../..

PWD=$(shell pwd)

all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -f *.o *.ko *.mod.* *.mod
