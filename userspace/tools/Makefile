TOP_DIR = ..

CFLAGS	=
LDFLAGS = -lcrypt
INC	= $(TOP_DIR)/../linux-2.6/include \
	  $(TOP_DIR)/include

SRCS	= filter.c login.c swctl.c reboot.c

SIMPLE_TARGETS = filter login reboot
TARGETS = $(SIMPLE_TARGETS) swctl

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

swctl: swctl.o
	$(Q) $(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $^ ../lib/if_generic.o ../lib/netlink.o ../lib/util.o \
		../lib/mm.o ../lib/shared.o -lrt -lreadline
	
$(SIMPLE_TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
