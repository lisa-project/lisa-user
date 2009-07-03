TOP_DIR = ..

CFLAGS  = -fPIC -g
INC	= $(TOP_DIR)/../linux-2.6/include \
	  $(TOP_DIR)/include

SRCS	= cdp_client.c if_generic.c mm.c util.c shared.c netlink.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: $(OBJS)

dep: $(DEPS)
