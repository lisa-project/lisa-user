TOP_DIR = ..

CFLAGS	= -DLiSA

ifneq ($(MAKECMDGOALS), clean)
$(info ********* Building testing tools *********)
endif

LDFLAGS = -lcrypt
include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/switch/Makefile.switch
INC	= $(LINUX_INC) \
	  $(SWITCH_INC) \
	  $(TOP_DIR)/include

SRCS	= filter.c login.c swctl.c swclid.c reboot.c

SIMPLE_TARGETS = filter login reboot swclid
TARGETS = $(SIMPLE_TARGETS) swctl

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

swctl: swctl.o $(TOP_DIR)/switch/libswitch.so
	$(Q) $(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) -o $@ $^ ../lib/if_generic.o \
		../lib/netlink.o ../lib/util.o ../lib/mm.o -lrt $(LDFLAGS)
	
$(SIMPLE_TARGETS): % : %.o
	$(QUIET_LD)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
