TOP_DIR = ..

CFLAGS  = -fPIC -g -I../include
LDFLAGS = -lpcap

include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/switch/Makefile.switch
INC	= $(LINUX_INC) \
	  $(SWITCH_INC) \
	  $(TOP_DIR)/include

SRCS	= vector.c rstp_client.c cdp_client.c if_generic.c mm.c util.c netlink.c socket_api.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: sock_api $(OBJS)

sock_api:
	$(QUIET_LD)$(CC) $(CFLAGS) socket_api.c -c $(LDFLAGS)

dep: $(DEPS)
