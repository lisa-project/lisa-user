ifndef CONFIG_SWITCH
EXTERNAL_BUILD=y
CONFIG_SWITCH=m
endif

obj-$(CONFIG_SWITCH) += switch.o

# KVER := $(shell uname -r)
# KSRC := /lib/modules/$(KVER)/build
KSRC := ../..

PWD=$(shell pwd)

all: modules

modules:
	$(MAKE) -C $(KSRC) SUBDIRS=$(PWD) MODVERDIR=$(PWD) modules

clean:
	rm -f *.o *.ko *.mod.*
