TOP_DIR		:= $(shell pwd)/../../
CFLAGS		= -fPIC -g -I..



include $(TOP_DIR)/scripts/Makefile.includes
INC		= $(LINUX_INC) \
		  $(TOP_DIR)/include \
		  $(TOP_DIR)/swcli

# LiSA Middleware
SRCS = lisa.c

include $(TOP_DIR)/scripts/Makefile.variables
include $(TOP_DIR)/scripts/Makefile.rules

.DEFAULT_GOAL = all

all: $(OBJS)

dep: $(DEPS)
