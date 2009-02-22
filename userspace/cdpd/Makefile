TOP_DIR = ..

CFLAGS  = -fPIC
LDFLAGS = -lpthread -lm -lrt

INC 	= $(TOP_DIR)/../linux-2.6/include \
	  $(TOP_DIR)/include
DEF	= _XOPEN_SOURCE=500 \
	  DEBUG

SRCS	= cdpd.c cdp_aging.c cdp_configuration.c cdp_send.c
TARGETS = cdpd

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

cdpd: $(OBJS)
	$(Q)$(MAKE) -C $(TOP_DIR)/lib
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $^ ../lib/util.o ../lib/if_generic.o ../lib/netlink.o
