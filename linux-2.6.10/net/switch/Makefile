obj-$(CONFIG_SWITCH) += switch.o

switch-objs := sw.o sw_fdb.o sw_proc.o sw_vdb.o sw_forward.o sw_ioctl.o

EXTRA_CFLAGS += -DDEBUG -g

# KDIR := /lib/modules/$(KVER)/build
KDIR := ../..

PWD=$(shell pwd)

all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -f *.o *.ko *.mod.* *.mod
