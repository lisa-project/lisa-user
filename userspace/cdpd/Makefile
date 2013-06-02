TOP_DIR = ..

CFLAGS  = -fPIC -g
LDFLAGS = -lpthread -lm -I../include -lpcap -lrt -pthread -L$(TOP_DIR)/switch -lswitch

ifneq ($(MAKECMDGOALS), clean)
$(info ********* Building the CDP daemon *********)
endif

include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/switch/Makefile.switch
INC 	= $(LINUX_INC) \
	  $(SWITCH_INC) \
	  $(TOP_DIR)/include
DEF	= DEBUG

SRCS	= cdpd.c cdp_aging.c cdp_configuration.c cdp_send.c
TARGETS = cdpd

all: $(TARGETS)

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

ifeq ($(IMPL), lisa)
CFLAGS	+= -DLiSA
endif

dep: $(DEPS)

LIB_OBJS = $(TOP_DIR)/lib/util.o \
	   $(TOP_DIR)/lib/mm.o \
	   $(TOP_DIR)/lib/if_generic.o \
	   $(TOP_DIR)/lib/netlink.o \
	   $(TOP_DIR)/lib/socket_api.o


$(TOP_DIR)/include/swsock.h:
	$(Q)$(MAKE) -C $(TOP_DIR)/include

$(LIB_OBJS): $(TOP_DIR)/lib/util.c $(TOP_DIR)/lib/if_generic.c
	$(Q)$(MAKE) -C $(TOP_DIR)/lib


cdpd: $(TOP_DIR)/include/swsock.h $(LIB_OBJS) $(OBJS)
	$(QUIET_LD)$(CC) -o $@ $(LIB_OBJS) $(OBJS) $(LDFLAGS)
