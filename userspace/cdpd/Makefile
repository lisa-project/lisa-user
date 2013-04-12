TOP_DIR = ..

CFLAGS  = -fPIC -g
LDFLAGS = -lpthread -lm -lrt

include $(TOP_DIR)/scripts/Makefile.includes
INC 	= $(LINUX_INC) \
	  $(TOP_DIR)/switch \
	  $(TOP_DIR)/switch/lisa \
	  $(TOP_DIR)/include
DEF	= _XOPEN_SOURCE=500 \
	  DEBUG

SRCS	= cdpd.c cdp_aging.c cdp_configuration.c cdp_send.c
TARGETS = cdpd

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

dep: $(DEPS)

LIB_OBJS = $(TOP_DIR)/lib/util.o $(TOP_DIR)/switch/lisa/lisa.o $(TOP_DIR)/switch/switch.o $(TOP_DIR)/lib/mm.o \
		   $(TOP_DIR)/lib/if_generic.o $(TOP_DIR)/lib/netlink.o

$(TOP_DIR)/include/swsock.h:
	$(Q)$(MAKE) -C $(TOP_DIR)/include

$(LIB_OBJS): $(TOP_DIR)/lib/util.c $(TOP_DIR)/lib/if_generic.c
	$(Q)$(MAKE) -C $(TOP_DIR)/lib

cdpd: $(TOP_DIR)/include/swsock.h $(LIB_OBJS) $(OBJS)
	$(QUIET_LD)$(CC) $(LDFLAGS) -o $@ $(LIB_OBJS) $(OBJS) 
