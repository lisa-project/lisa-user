TOP_DIR = ..

CFLAGS  = -fPIC -g

include $(TOP_DIR)/scripts/Makefile.includes
include $(TOP_DIR)/switch/Makefile.switch
INC	= $(LINUX_INC) \
	  $(SWITCH_INC) \
	  $(TOP_DIR)/include

SRCS	= vector.c rstp_client.c cdp_client.c if_generic.c mm.c util.c netlink.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: $(OBJS)

dep: $(DEPS)
