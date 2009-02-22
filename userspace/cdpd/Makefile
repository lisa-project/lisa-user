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

LIB_OBJS = $(TOP_DIR)/lib/util.o $(TOP_DIR)/lib/if_generic.o $(TOP_DIR)/lib/netlink.o

$(TOP_DIR)/include/swsock.h:
	$(Q)$(MAKE) -C $(TOP_DIR)/include

$(LIB_OBJS): $(TOP_DIR)/lib/util.c $(TOP_DIR)/lib/if_generic.c
	$(Q)$(MAKE) -C $(TOP_DIR)/lib

cdpd: $(TOP_DIR)/include/swsock.h $(LIB_OBJS) $(OBJS)
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $(LIB_OBJS) $(OBJS) 
