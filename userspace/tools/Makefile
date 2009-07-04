TOP_DIR = ..

CFLAGS	=
LDFLAGS = -lcrypt
include $(TOP_DIR)/scripts/Makefile.includes
INC	= $(LINUX_INC) \
	  $(TOP_DIR)/include

SRCS	= filter.c login.c swctl.c swclid.c reboot.c

SIMPLE_TARGETS = filter login reboot swclid
TARGETS = $(SIMPLE_TARGETS) swctl

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

swctl: swctl.o
	$(Q) $(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $^ ../lib/if_generic.o ../lib/netlink.o ../lib/util.o \
		../lib/mm.o ../lib/shared.o -lrt
	
$(SIMPLE_TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
