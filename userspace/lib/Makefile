TOP_DIR = ..

CFLAGS  = -fPIC -g

include $(TOP_DIR)/scripts/Makefile.includes
INC	= $(LINUX_INC) \
	  $(TOP_DIR)/include

SRCS	= cdp_client.c if_generic.c mm.c util.c shared.c netlink.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: $(OBJS)

dep: $(DEPS)
